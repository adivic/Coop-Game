// Fill out your copyright notice in the Description page of Project Settings.


#include "SGrenadeActor.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundCue.h"

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
}

// Called when the game starts or when spawned
void ASGrenadeActor::BeginPlay()
{
	Super::BeginPlay();
	FTimerHandle ExplodeTimer;
	GetWorldTimerManager().SetTimer(ExplodeTimer, this, &ASGrenadeActor::Explode, 3.f, false);
}

void ASGrenadeActor::Explode_Implementation() {
	FTransform SpawnTransform = FTransform(GetActorRotation(), GetActorLocation(), FVector(5));
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, SpawnTransform);
	TArray<AActor*> IgnoreActors;
	UGameplayStatics::ApplyRadialDamage(GetWorld(), 100.f, GetActorLocation(), DamageRadius, DamageType, IgnoreActors, GetInstigator());

	UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSound, GetActorLocation());

	Destroy();
}
