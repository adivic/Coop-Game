// Fill out your copyright notice in the Description page of Project Settings.

#include "SCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "CoopGame/Public/SWeapon.h"
#include "CoopGame/CoopGame.h"
#include "CoopGame/Public/Components/SHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimSequenceBase.h"
#include "DrawDebugHelpers.h"
#include "SCollectable.h"
#include "Sound/SoundCue.h"
#include "Kismet/GameplayStatics.h"
#include "CoopGame/Public/Components/SThrowableComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SGrenadeActor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "Components/SkinnedMeshComponent.h"

// Sets default values
ASCharacter::ASCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SprintArmComp"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->bUsePawnControlRotation = true;

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);

	HealthComponent = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComponent"));
	ThrowableComponent = CreateDefaultSubobject<USThrowableComponent>(TEXT("ThrowableComponent"));

	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(SpringArmComp);
	
	ZoomedFOV = 65.f;
	ZoomInterpSpeed = 20;
	WeaponAttachSocket = "WeaponSocket";
	DefaultWalkSpeed = 400.f;
}

// Called when the game starts or when spawned
void ASCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	DefaultFOV = PlayerCamera->FieldOfView;
	HealthComponent->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);

	auto MapName = GetWorld()->GetMapName();
	if (GetLocalRole() == ROLE_Authority && !MapName.Contains("Lobby")) {
		//Spawn default weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CurrentWeapon = GetWorld()->SpawnActor<ASWeapon>(StarterWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (CurrentWeapon) {
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocket);
		}
	}
}

void ASCharacter::MoveForward(float value) {
	AddMovementInput(GetActorForwardVector() * value);
}

void ASCharacter::MoveRight(float value) {
	if(!bIsSprint) AddMovementInput(GetActorRightVector() * value);
}

//TODO Toggle crouch
void ASCharacter::BeginCrouch() {
	bIsCrouched ? UnCrouch() : Crouch();
}

void ASCharacter::StartFire() {
	if (CurrentWeapon && !bIsSprint) {
		CurrentWeapon->StartFire();
	}
}

void ASCharacter::StopFire() {
	if (CurrentWeapon) {
		CurrentWeapon->StopFire();
	}
}

ASWeapon* ASCharacter::GetWeapon() const {
	return CurrentWeapon;
}

void ASCharacter::ServerStartADS_Implementation() {
	if (GetLocalRole() < ROLE_Authority) {
		ServerStartADS();
	}
	bWantsToZoom = true;
}

bool ASCharacter::ServerStartADS_Validate() {
	return true;
}

void ASCharacter::ServerStopADS_Implementation() {
		if (GetLocalRole() < ROLE_Authority) {
		ServerStopADS();
	}
	bWantsToZoom = false;
}

bool ASCharacter::ServerStopADS_Validate() {
	return true;
}

void ASCharacter::OnHealthChanged(USHealthComponent* HealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser) {
	if (Health <= 0.0f && !bDied) {
		bDied = true;

		GetMovementComponent()->StopMovementImmediately();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CurrentWeapon->StopFire();
		DetachFromControllerPendingDestroy();
		auto Messh = Cast<USkinnedMeshComponent>(GetMesh());
		if (Messh) 
			Messh->HideBoneByName("head", EPhysBodyOp::PBO_None);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FTransform SpawnTransform = FTransform(GetMesh()->GetSocketRotation("head"), GetMesh()->GetSocketLocation("head"), FVector(0.05f));
		auto headMesh = GetWorld()->SpawnActor<AActor>(BodyPart, SpawnTransform, Params);
		headMesh->SetLifeSpan(15.f);

		if (!bIsRagdoll) {
			OnRep_SetupRagdoll();
		}
		bIsRagdoll = true;

		SetLifeSpan(15.0f);
	}
}

void ASCharacter::ServerStartSprint_Implementation() {
	bIsSprint = true;
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed * 1.4f;
}

bool ASCharacter::ServerStartSprint_Validate() {
	return true;
}

void ASCharacter::ServerStopSprint_Implementation() {
	bIsSprint = false;
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
}

bool ASCharacter::ServerStopSprint_Validate() {
	return true;
}

void ASCharacter::OnRep_SetupRagdoll() {
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	SetActorEnableCollision(true);

	//SetRagdoll
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();
	GetMesh()->bBlendPhysics = true;
}

void ASCharacter::Reload() {
	StopFire();
	auto Info = CurrentWeapon->GetAmmunitionInfo();
	if (Info.MaxAmmo > 0 && Info.CurrentAmmo != Info.FullClip) {
		ServerPlayReloadMontage();
	}
	CurrentWeapon->Reload();
}

void ASCharacter::MulticastPlayMontage_Implementation(UAnimMontage* MontageToPlay) {
	USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(GetMesh());
	if (SkeletalMesh) {
		SkeletalMesh->GetAnimInstance()->Montage_Play(MontageToPlay);
	}
}

void ASCharacter::ServerPlayReloadMontage_Implementation() {
	if (GetLocalRole() == ROLE_Authority) 
		MulticastPlayMontage(ReloadAnim);
	else
		ServerPlayReloadMontage();
}

void ASCharacter::Pickup_Implementation() {
	bFPressed = !bFPressed;
	if (GetLocalRole() < ROLE_Authority) Pickup();
	
	if (bFPressed) {
		FVector StartLocation;
		FRotator EyeRotator;
		GetActorEyesViewPoint(StartLocation, EyeRotator);
		FVector EndLocation = StartLocation + (EyeRotator.Vector() * 350.f);

		FHitResult Hit;
		FCollisionShape Shape;
		Shape.SetCapsule(40, 80);
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		GetWorld()->SweepSingleByChannel(Hit, GetActorLocation(), GetActorLocation() + (GetActorForwardVector() * 50), FQuat::Identity, ECC_Visibility, Shape, Params);
		
		ASCollectable* PickupActor = Cast<ASCollectable>(Hit.Actor);
		if (PickupActor && PickupActor->GetIsOverlap()) {
			switch (PickupActor->GetType()) {
			case ECollectableType::AMMO:
				if (!CurrentWeapon->IsFull()) {
					CurrentWeapon->RefilAmmo();
					PickupActor->Destroy();
					UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
				} else {
					UGameplayStatics::PlaySoundAtLocation(this, NegativeSound, GetActorLocation());
				}
				break;
			case ECollectableType::MAX_AMMO:
				if (!CurrentWeapon->IsFull()) {
					CurrentWeapon->RefilAmmo(true);
					PickupActor->Destroy();
					UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
				} else {
					UGameplayStatics::PlaySoundAtLocation(this, NegativeSound, GetActorLocation());
				}
				break;

			case ECollectableType::HELATH:
				if (HealthComponent->GetHealth() != 100) {
					HealthComponent->Heal(20.f);
					PickupActor->Destroy();
					UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
				} else {
					UGameplayStatics::PlaySoundAtLocation(this, NegativeSound, GetActorLocation());
				}
				break;
			}
		}
	}
}

void ASCharacter::Jump() {
	if (CanVault()) {
		Vault();
	} else {
		Super::Jump();
	}
}

void ASCharacter::Vault_Implementation() {
	MulticastPlayMontage(VaultAnim);
	FVector EndLocation = GetActorLocation() + (GetActorUpVector() * 50) + (GetActorForwardVector() * 300);
	FVector NewLocation = FMath::VInterpTo(GetActorLocation(), EndLocation, GetWorld()->GetDeltaSeconds(), 20.f);
	NewLocation += FVector(0, 0, GetDefaultHalfHeight());
	FLatentActionInfo LatInfo;
	LatInfo.CallbackTarget = this;
	UKismetSystemLibrary::MoveComponentTo(GetCapsuleComponent(), NewLocation, FRotator::ZeroRotator, false, true, .1f, false, EMoveComponentAction::Type::Move, LatInfo);
}

bool ASCharacter::CanVault() const {
	FHitResult Hit;
	FHitResult HitFalse;
	FVector EndLocation = GetActorLocation() + (GetActorForwardVector() * 100);

	FCollisionQueryParams Params; 
	Params.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), EndLocation, ECC_Visibility, Params);
	GetWorld()->LineTraceSingleByChannel(HitFalse, GetActorLocation()+ FVector(0,0,50), EndLocation + FVector(0, 0, 50), ECC_Visibility, Params);
	
	if (Hit.bBlockingHit && !HitFalse.bBlockingHit) {
		if (Hit.GetComponent() != nullptr) {
			return true;
		}
	}
	return false;
}

/*bool ASCharacter::CanClimb() {
	
	FName HeadSocket = "head";
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FVector StartLocation = GetMesh()->GetSocketLocation(HeadSocket);
	FVector EndLocation = GetMesh()->GetSocketLocation(HeadSocket) + (GetActorForwardVector() * 100);
	GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, ECC_Visibility, Params);

	if (Hit.bBlockingHit) {
		if (Hit.GetComponent() != nullptr) {
			return true;
		}
	}
	return false;
}

void ASCharacter::Climb_Implementation() {
	MulticastPlayMontage(ClimbAnim);
	FHitResult Hit;
	GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation() + FVector(0, 0, 500), GetActorLocation() + (GetActorForwardVector() * 130), ECC_Visibility);
	FVector Loc = Hit.Location;

	FTimerHandle Handle;
	FTimerDelegate RespawnDelegate = FTimerDelegate::CreateUObject(this, &ASCharacter::Climbed, Loc);
	GetWorldTimerManager().SetTimer(Handle, RespawnDelegate, 2.4f, false);
}

void ASCharacter::Climbed_Implementation(FVector ClimbLocation) {
	FLatentActionInfo LatInfo;
	LatInfo.CallbackTarget = this;
	UKismetSystemLibrary::MoveComponentTo(GetCapsuleComponent(), ClimbLocation, FRotator::ZeroRotator, false, true, .1f, false, EMoveComponentAction::Type::Move, LatInfo);
}*/

// Called every frame
void ASCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float TargetFOV = bWantsToZoom ? ZoomedFOV : DefaultFOV;

	float NewFov = FMath::FInterpTo(PlayerCamera->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);

	PlayerCamera->SetFieldOfView(NewFov);

	if (!IsLocallyControlled()) {
		auto Pitch = RemoteViewPitch * 360.f / 255.f;
		AimPitch = FMath::ClampAngle(Pitch, -90, 90);
	} else {
		FRotator DeltaRot = GetControlRotation() - GetActorRotation();
		DeltaRot.Normalize();
		AimPitch = FMath::FInterpTo(AimPitch, FMath::ClampAngle(DeltaRot.Pitch, -90, 90), DeltaTime, 20);
	}
}

// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &ASCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASCharacter::Jump);

	PlayerInputComponent->BindAction("ADS", IE_Pressed, this, &ASCharacter::ServerStartADS);
	PlayerInputComponent->BindAction("ADS", IE_Released, this, &ASCharacter::ServerStopADS);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopFire);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ASCharacter::ServerStartSprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ASCharacter::ServerStopSprint);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ASCharacter::Reload);

	PlayerInputComponent->BindAction("Pickup", IE_Pressed, this, &ASCharacter::Pickup);
	PlayerInputComponent->BindAction("Pickup", IE_Released, this, &ASCharacter::Pickup);
}

FVector ASCharacter::GetPawnViewLocation() const {
	if (PlayerCamera) {
		return PlayerCamera->GetComponentLocation();
	}
	return Super::GetPawnViewLocation();
}

void ASCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCharacter, CurrentWeapon);
	DOREPLIFETIME(ASCharacter, bDied);
	DOREPLIFETIME(ASCharacter, bWantsToZoom);
	DOREPLIFETIME(ASCharacter, AimPitch);
	DOREPLIFETIME(ASCharacter, bIsSprint);
	DOREPLIFETIME(ASCharacter, bIsRagdoll);
	DOREPLIFETIME(ASCharacter, ReloadAnim);
	DOREPLIFETIME(ASCharacter, VaultAnim);
}
