// Fill out your copyright notice in the Description page of Project Settings.

#include "..\..\Public\AI\STrackerBot.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"
#include "NavigationPath.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "../../Public/Components/SHealthComponent.h"
#include "Components/SphereComponent.h"
#include "../../Public/SCharacter.h"
#include "Sound/SoundCue.h"

static int32 DebugTrackerBotDrawing = 0;
FAutoConsoleVariableRef CVARDebugTrackerBotDrawing(
	TEXT("COOP.DebugTrackerBot"),
	DebugTrackerBotDrawing,
	TEXT("Draw Debug Lines for TrackerBot"),
	ECVF_Cheat);


// Sets default values
ASTrackerBot::ASTrackerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetSimulatePhysics(true);
	RootComponent = MeshComp;

	HealthComponent = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComponent"));

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(200);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComp->SetupAttachment(RootComponent);
	
	bUseVelocityChanged = false;
	MovementForce = 1000;
	RequiredDistanceToTarget = 100;

	ExplosionDamage = 60;
	ExplosionRadius = 350;

	SelfDamageInterval = 0.25f;
}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	
	HealthComponent->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);
	
	if (GetLocalRole() == ROLE_Authority) {
		//Find initial move-to
		NextPathPoint = GetNextPathPoint();

		FTimerHandle TimerHandle_CheckBots;
		GetWorldTimerManager().SetTimer(TimerHandle_CheckBots, this, &ASTrackerBot::CheckNearbyBots, true,1.f);
	}
}

void ASTrackerBot::HandleTakeDamage(USHealthComponent* HealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser) {
	
	if (MatInst == nullptr) {
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst) {
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}

	if (Health <= 0.0f) {
		SelfDestruct();
	}
}

FVector ASTrackerBot::GetNextPathPoint() {

	AActor* BestTarget = nullptr;
	float NearestTargetDistance = FLT_MAX;

	for (FConstPawnIterator it = GetWorld()->GetPawnIterator(); it; ++it) {
		APawn* TestPawn = it->Get();
		if (TestPawn == nullptr || USHealthComponent::IsFriendly(TestPawn, this)) {
			continue;
		}
		USHealthComponent*TestPawnHealthComp = Cast<USHealthComponent>(TestPawn->GetComponentByClass(USHealthComponent::StaticClass()));
		if (TestPawnHealthComp && TestPawnHealthComp->GetHealth() > 0.0f) {
			
			float Distance = (TestPawn->GetActorLocation() - GetActorLocation()).Size();
			if (Distance < NearestTargetDistance) {
				BestTarget = TestPawn;
				NearestTargetDistance = Distance;
			}
		}
	}

	if (BestTarget) {
		auto NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), BestTarget);

		GetWorldTimerManager().ClearTimer(TimerHandle_RefreshPath);
		GetWorldTimerManager().SetTimer(TimerHandle_RefreshPath, this, &ASTrackerBot::RefreshPath, 5.f, false);

		if (NavPath && NavPath->PathPoints.Num() > 1) {
			//return next point in the path
			return NavPath->PathPoints[1];
		}
	}
	
	// Failed to find path
	return GetActorLocation();
}

void ASTrackerBot::SelfDestruct() {
	if (bExploded) return;

	bExploded = true;
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
	UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());
	
	MeshComp->SetVisibility(false, true);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (GetLocalRole() == ROLE_Authority) {
		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(this);

		float RealDamage = ExplosionDamage + (ExplosionDamage * PowerLevel);

		if (DebugTrackerBotDrawing) {
			DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 2.f, 0, 1.f);
		}

		UGameplayStatics::ApplyRadialDamage(this, RealDamage, GetActorLocation(), ExplosionRadius, nullptr, IgnoredActors, this, GetInstigatorController(), true);

		SetLifeSpan(2.0f);
	}
	
}

void ASTrackerBot::DamageSelf() {
	UGameplayStatics::ApplyDamage(this, 20, GetInstigatorController(), this, nullptr);
}

void ASTrackerBot::CheckNearbyBots() {
	
	const float Radius = 600.f;

	FCollisionShape CollShape;
	CollShape.SetSphere(Radius);

	TArray<FOverlapResult> Overlaps;

	GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn, CollShape);
	
	if (DebugTrackerBotDrawing) {
		DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 12, FColor::White, false, 1.f);
	}

	int32 NumOfBots = 0;

	for (FOverlapResult Result: Overlaps) {
		ASTrackerBot* Bot = Cast<ASTrackerBot>(Result.GetActor());
		if (Bot && Bot != this) {
			NumOfBots++;
		}
	}
	PowerLevel = FMath::Clamp(NumOfBots, 0, MaxPowerLevel);

	if (MatInst == nullptr) {
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}
	if (MatInst) {
		float Alpha = PowerLevel / (float) MaxPowerLevel;

		MatInst->SetScalarParameterValue("PowerLevelAlpha", Alpha);
	}
}

void ASTrackerBot::RefreshPath() {
	NextPathPoint = GetNextPathPoint();
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() == ROLE_Authority && !bExploded) {
		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

		if (DistanceToTarget <= RequiredDistanceToTarget) {
			NextPathPoint = GetNextPathPoint();

			if (DebugTrackerBotDrawing) {
				DrawDebugString(GetWorld(), GetActorLocation(), TEXT("Target Reached"));
			}
			
		} else {
			// Keep moving towards next target
			FVector ForceDirection = NextPathPoint - GetActorLocation();
			ForceDirection.Normalize();
			ForceDirection *= MovementForce;
			MeshComp->AddForce(ForceDirection, NAME_None, bUseVelocityChanged);

			if (DebugTrackerBotDrawing) {
				DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32, FColor::Red, false, 0.0f, 0, 1.f);
			}
			
		}

		if (DebugTrackerBotDrawing) {
			DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Blue, false, 4.f, 1.f);
		}
		
	}
}

void ASTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor) {

	Super::NotifyActorBeginOverlap(OtherActor);

	if (!bStartedSelfDestruction && !bExploded) {
		ASCharacter* PlayerPawn = Cast<ASCharacter>(OtherActor);
		if (PlayerPawn && !USHealthComponent::IsFriendly(OtherActor, this)) {

			if (Role == ROLE_Authority) {
				GetWorldTimerManager().SetTimer(TimerHandler_SelfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 0.0f);
			}

			UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);

			bStartedSelfDestruction = true;
		}
	}
}
