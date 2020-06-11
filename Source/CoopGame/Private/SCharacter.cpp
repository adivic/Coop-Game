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
#include "GameFramework/CharacterMovementComponent.h"

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

	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(SpringArmComp);

	ZoomedFOV = 65.f;
	ZoomInterpSpeed = 20;
	WeaponAttachSocket = "WeaponSocket";
	DefaultWalkSpeed =400.f;
}

// Called when the game starts or when spawned
void ASCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	DefaultFOV = PlayerCamera->FieldOfView;
	HealthComponent->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);

	if (GetLocalRole() == ROLE_Authority) {
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
	AddMovementInput(GetActorRightVector() * value);
}

//TODO Toggle crouch
void ASCharacter::BeginCrouch() {
	Crouch();
}

void ASCharacter::EndCrouch() {
	UnCrouch();
}

void ASCharacter::StartFire() {
	if (CurrentWeapon) {
		CurrentWeapon->StartFire();
	}
}

void ASCharacter::StopFire() {
	if (CurrentWeapon) {
		CurrentWeapon->StopFire();
	}
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

		if (!IsRagdoll) {
			OnRep_SetupRagdoll();
		}
		IsRagdoll = true;

		SetLifeSpan(15.0f);
	}
}

void ASCharacter::ServerStartSprint_Implementation() {
	if (GetLocalRole() < ROLE_Authority) {
		ServerStartSprint();
	}
	IsSprint = true;
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed * 1.4f;
}

bool ASCharacter::ServerStartSprint_Validate() {
	return true;
}

void ASCharacter::ServerStopSprint_Implementation() {
	if (GetLocalRole() < ROLE_Authority) {
		ServerStopSprint();
	}
	IsSprint = false;
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
		ServerPlayMontage();
	}
	CurrentWeapon->Reload();
}

void ASCharacter::MulticastPlayMontage_Implementation() {
	USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(GetMesh());
	if (SkeletalMesh) {
		SkeletalMesh->GetAnimInstance()->Montage_Play(ReloadAnim);
	}
}

void ASCharacter::ServerPlayMontage_Implementation() {
	if(GetLocalRole() == ROLE_Authority)
		MulticastPlayMontage();
}

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
		//AimPitch = FMath::ClampAngle(DeltaRot.Pitch, -90, 90);
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
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	PlayerInputComponent->BindAction("ADS", IE_Pressed, this, &ASCharacter::ServerStartADS);
	PlayerInputComponent->BindAction("ADS", IE_Released, this, &ASCharacter::ServerStopADS);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ASCharacter::StartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::StopFire);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ASCharacter::ServerStartSprint);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ASCharacter::ServerStopSprint);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ASCharacter::Reload);
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
	DOREPLIFETIME(ASCharacter, IsSprint);
	DOREPLIFETIME(ASCharacter, IsRagdoll);
}
