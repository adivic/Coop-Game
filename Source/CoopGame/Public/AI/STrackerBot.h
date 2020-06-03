// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "STrackerBot.generated.h"

class USHealthComponent;
class UMaterialInstanceDynamic;
class USphereComponent;
class USoundCue;

UCLASS()
class COOPGAME_API ASTrackerBot : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASTrackerBot();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, Category = Components)
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleDefaultsOnly, Category = Components)
	USHealthComponent* HealthComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = Components)
	USphereComponent* SphereComp;

	UFUNCTION()
	void HandleTakeDamage(USHealthComponent* HealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	FVector GetNextPathPoint();

	//Next Point in navigation path
	FVector NextPathPoint;

	UPROPERTY(EditDefaultsOnly, Category = TrackerBot)
	float MovementForce;

	UPROPERTY(EditDefaultsOnly, Category = TrackerBot)
	bool bUseVelocityChanged;

	UPROPERTY(EditDefaultsOnly, Category = TrackerBot)
	float RequiredDistanceToTarget;

	// Dynamic Material 
	UMaterialInstanceDynamic* MatInst;

	void SelfDestruct();

	UPROPERTY(EditDefaultsOnly, Category = TrackerBot)
	UParticleSystem* ExplosionEffect;

	bool bExploded;

	bool bStartedSelfDestruction= false;

	UPROPERTY(EditDefaultsOnly, Category = TrackerBot)
	float ExplosionRadius;

	UPROPERTY(EditDefaultsOnly, Category = TrackerBot)
	float ExplosionDamage;

	FTimerHandle TimerHandler_SelfDamage;

	UPROPERTY(EditDefaultsOnly, Category = TrackerBot)
	float SelfDamageInterval;

	void DamageSelf();

	UPROPERTY(EditDefaultsOnly, Category = TrackerBot)
	USoundCue* SelfDestructSound;

	UPROPERTY(EditDefaultsOnly, Category = TrackerBot)
	USoundCue* ExplodeSound;

	void CheckNearbyBots();

	UPROPERTY(EditDefaultsOnly, Category = TracerBot)
	int32 MaxPowerLevel = 4;

	int32 PowerLevel = 0;

	FTimerHandle TimerHandle_RefreshPath;

	void RefreshPath();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

};
