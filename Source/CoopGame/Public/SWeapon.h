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

USTRUCT(BlueprintType)
struct FAmmunition {
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentAmmo;
	UPROPERTY(BlueprintReadOnly)
	int32 FullClip;
	UPROPERTY(BlueprintReadOnly)
	int32 MaxAmmo;
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

	UPROPERTY(EditDefaultsOnly, Category = Sound)
	USoundCue* HeadshotSound;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = Weapon)
	FAmmunition Ammunition;

	int32 DefaultMaxAmmo;

public:

	virtual void Fire();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Weapon)
	void OnFire();

	void StartFire();

	void StopFire();

	UFUNCTION(Server, Reliable, WithValidation)
	void Reload();

	FAmmunition GetAmmunitionInfo() const;

	void RefilAmmo(bool bIsMaxAmmo = false);

	FORCEINLINE bool IsFull() const { return Ammunition.MaxAmmo == DefaultMaxAmmo; }
};

