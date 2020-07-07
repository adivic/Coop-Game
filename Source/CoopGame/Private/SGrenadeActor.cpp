// Fill out your copyright notice in the Description page of Project Settings.


#include "SGrenadeActor.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"
#include "SCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Components/SphereComponent.h"

// Sets default values
ASGrenadeActor::ASGrenadeActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetMassOverrideInKg(NAME_None, 1.f);
	RootComponent = MeshComp;

	SetReplicates(true);
	SetReplicateMovement(true);
	DamageRadius = 300.f;
	bExploded = false;
}

// Called when the game starts or when spawned
void ASGrenadeActor::BeginPlay()
{
	Super::BeginPlay();
	FTimerHandle ExplodeTimer;
	GetWorldTimerManager().SetTimer(ExplodeTimer, this, &ASGrenadeActor::Exploding, 3.f, false);
}

//Server
void ASGrenadeActor::Explode_Implementation() {
	OnRep_Exploded();
	//Apply Damage
	FHitResult Hit;
	FCollisionShape ShapeShpere;
	ShapeShpere.Sphere;
	ShapeShpere.SetSphere(DamageRadius);
	FCollisionQueryParams Params;

	FCollisionObjectQueryParams QueryParams;
	QueryParams.AddObjectTypesToQuery(ECC_Pawn);
	GetWorld()->SweepSingleByObjectType(Hit, GetActorLocation(), GetActorLocation() * 300, FQuat::Identity, QueryParams, ShapeShpere, Params);

	if (Hit.bBlockingHit && Hit.Actor != nullptr) {
		ASCharacter* DamagedChar = Cast<ASCharacter>(Hit.GetActor());
		UGameplayStatics::ApplyRadialDamage(Hit.GetActor(), 100, GetActorLocation(), 300, DamageType, TArray<AActor*>(), GetInstigator(), GetInstigatorController(), true);
		//UGameplayStatics::ApplyDamage(Hit.GetActor(), 110.f, GetInstigatorController(), GetInstigator(), DamageType);

		UE_LOG(LogTemp, Warning, TEXT("HIT is True %s"), *Hit.GetActor()->GetName());
	}
	
	DrawDebugSphere(GetWorld(), GetActorLocation(), DamageRadius, 20, FColor::Red, false, 3.f);
	SetLifeSpan(1.0f);
}

void ASGrenadeActor::Exploding() {
	bExploded = true;
	Explode();
	OnRep_Exploded();
}

void ASGrenadeActor::OnRep_Exploded() {
	FTransform SpawnTransform = FTransform(GetActorRotation(), GetActorLocation(), FVector(5));
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, SpawnTransform);
	MeshComp->SetHiddenInGame(true);
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSound, GetActorLocation());
}

void ASGrenadeActor::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASGrenadeActor, bExploded);
}