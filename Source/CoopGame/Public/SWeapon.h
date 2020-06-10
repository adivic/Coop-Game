// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SWeapon.generated.h"

class USkeletalMeshComponent;
class UParticleSystem;

//Contains informations of single line trace
USTRUCT()
struct FHitScanTrace
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TEnumAsByte<EPhysicalSurface> SurfaceType;
	
	UPROPERTY()
	FVector_NetQuantize TraceTo;
};

UCLASS()
class COOPGAME_API ASWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASWeapon();

protected:

	virtual void BeginPlay() override;

	void PlayFireEffects(FVector TraceEndPoint);

	void PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
	USkeletalMeshComponent* MeshComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	TSubclassOf<class UDamageType> DamageType;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	FName MuzzleSocket;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	UParticleSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	UParticleSystem* DefaultImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	UParticleSystem* FleshImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	UParticleSystem* TracerEffect;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	FName TracerTargetName;

	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	TSubclassOf<class UCameraShake> FireCameraShake;

	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	float BaseDamage;

	/*Bullet Spread in degrees*/
	UPROPERTY(EditDefaultsOnly, Category = Weapon, meta = (ClampMin=0.0f))
	float BulletSpread;

	FTimerHandle TimerHandler_FireRate;

	float LastFiredTime;

	/*RPM bullets per minute*/
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	float FireRate;

	//Derived from FireRate
	float TimeBetweenShots;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire();

	UPROPERTY(ReplicatedUsing=OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

	UFUNCTION()
	void OnRep_HitScanTrace();

	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	class USoundCue* FiringSound;

public:

	virtual void Fire();

	void StartFire();

	void StopFire();

};

