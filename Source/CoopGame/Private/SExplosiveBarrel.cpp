// Fill out your copyright notice in the Description page of Project Settings.


#include "SExplosiveBarrel.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "CoopGame/Public/Components/SHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASExplosiveBarrel::ASExplosiveBarrel()
{
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetCollisionObjectType(ECC_PhysicsBody);
	RootComponent = MeshComp;

	HealthComponent = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComponent"));

	RadialForceComp = CreateDefaultSubobject<URadialForceComponent>(TEXT("RadialForceComp"));
	RadialForceComp->SetupAttachment(MeshComp);
	RadialForceComp->Radius = 200;
	RadialForceComp->bAutoActivate = false;
	RadialForceComp->bImpulseVelChange = true;
	RadialForceComp->bIgnoreOwningActor = true;
	ExplosionImpulse = 400;
	
	SetReplicates(true);
	SetReplicatingMovement(true);
}

// Called when the game starts or when spawned
void ASExplosiveBarrel::BeginPlay()
{
	Super::BeginPlay();
	
	HealthComponent->OnHealthChanged.AddDynamic(this, &ASExplosiveBarrel::OnHealthChanged);
}

void ASExplosiveBarrel::OnHealthChanged(USHealthComponent* HealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser) {
	
	if (bExploded) return;
	
	if (Health <= 0.0f) {
		bExploded = true;
		OnRep_Exploded();

		MeshComp->AddImpulse(FVector::UpVector * ExplosionImpulse, NAME_None, true);
		
		RadialForceComp->FireImpulse();
	}
}

void ASExplosiveBarrel::OnRep_Exploded() {
	//MeshComp->AddImpulse(FVector::UpVector * ExplosionImpulse, NAME_None, true);

	if (ExplosionEffect) {
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation(), GetActorRotation());
	}
	if (ExplodedMaterial) {
		MeshComp->SetMaterial(0, ExplodedMaterial);
	}
	//RadialForceComp->FireImpulse();
}

void ASExplosiveBarrel::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASExplosiveBarrel, bExploded);
}
