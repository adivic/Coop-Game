// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "CoopGame/CoopGame.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundCue.h"

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVARDebugWeaponDrawing(
	TEXT("COOP.DebugWeapons"),
	DebugWeaponDrawing,
	TEXT("Draw Debug Lines for Weapons"),
	ECVF_Cheat);


// Sets default values
ASWeapon::ASWeapon()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocket = "Muzzle";
	TracerTargetName = "Target";
	BaseDamage = 20.f;
	FireRate = 600.f;
	BulletSpread = 2.f;

	Ammunition.CurrentAmmo = 30;
	Ammunition.FullClip = 30;
	Ammunition.MaxAmmo = 100;

	DefaultMaxAmmo = Ammunition.MaxAmmo;

	SetReplicates(true);

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
}

void ASWeapon::BeginPlay() {
	Super::BeginPlay();

	TimeBetweenShots = 60 / FireRate;
	OnFire();
}

void ASWeapon::OnFire_Implementation() {
	// Override in Blueprint
}

void ASWeapon::Fire() {

	if (GetLocalRole() < ROLE_Authority) {
		ServerFire();
	}

	AActor* MyOwner = GetOwner();
	if (MyOwner && Ammunition.CurrentAmmo > 0) {

		//Ammo
		Ammunition.CurrentAmmo--;
		OnFire();

		FVector EyeLocation;
		FRotator EyeRotator;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotator);

		FVector ShotDirection = EyeRotator.Vector();

		//Bullet Spread
		float HalfRad = FMath::DegreesToRadians(BulletSpread);
		ShotDirection = FMath::VRandCone(ShotDirection, HalfRad, HalfRad);

		FVector TraceEnd = EyeLocation + (ShotDirection * 10000.f);
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		// Particle "target" comp
		FVector TracerEndPoint = TraceEnd;

		EPhysicalSurface SurfaceType = SurfaceType_Default;

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, COLLISION_WEAPON, QueryParams)) {
			AActor* HitActor = Hit.GetActor();

			TracerEndPoint = Hit.ImpactPoint;

			SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float ActualDamage = BaseDamage;
			if (SurfaceType == SURFACE_FLESHVULNERABLE) {
				ActualDamage *= 4.f;
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), HeadshotSound, GetActorLocation());
			}

			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), MyOwner, DamageType);

			PlayImpactEffects(SurfaceType, Hit.ImpactPoint);

		}

		if (DebugWeaponDrawing > 0) {
			DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::Red, false, 1.f, 0, 1.f);
		}
		PlayFireEffects(TracerEndPoint);

		if(GetLocalRole() == ROLE_Authority) {
			HitScanTrace.TraceTo = TracerEndPoint;
			HitScanTrace.SurfaceType = SurfaceType;
		}

		LastFiredTime = GetWorld()->TimeSeconds;
	}	
}

void ASWeapon::StartFire() {

	float FirstDelay = FMath::Max(LastFiredTime - TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);

	GetWorldTimerManager().SetTimer(TimerHandler_FireRate, this, &ASWeapon::Fire, TimeBetweenShots, true, FirstDelay);
}

void ASWeapon::StopFire() {
	GetWorldTimerManager().ClearTimer(TimerHandler_FireRate);
}

void ASWeapon::Reload_Implementation() {
	if (Ammunition.MaxAmmo > 0) {
		if (Ammunition.CurrentAmmo > 0) {
			int32 AmmoDifference = Ammunition.FullClip - Ammunition.CurrentAmmo;
			if (Ammunition.MaxAmmo - AmmoDifference >= 0) {
				Ammunition.CurrentAmmo = Ammunition.FullClip;
				Ammunition.MaxAmmo -= AmmoDifference;
			} else {
				Ammunition.CurrentAmmo += Ammunition.MaxAmmo;
				Ammunition.MaxAmmo = 0;
			}
		} else {
			if (Ammunition.MaxAmmo - Ammunition.FullClip > 0) {
				Ammunition.CurrentAmmo = Ammunition.FullClip;
				Ammunition.MaxAmmo -= Ammunition.FullClip;
			} else {
				Ammunition.CurrentAmmo = Ammunition.MaxAmmo;
				Ammunition.MaxAmmo = 0;
			}
		}
		OnFire();
	}
}

bool ASWeapon::Reload_Validate() {
	return true;
}

void ASWeapon::PlayFireEffects(FVector TraceEnd) {

	if (MuzzleEffect) {
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocket);
	}

	if (TracerEffect) {
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocket);
		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
		if (TracerComp) {
			TracerComp->SetVectorParameter(TracerTargetName, TraceEnd);
		}
		if(FiringSound)
			UGameplayStatics::PlaySoundAtLocation(this, FiringSound, MuzzleLocation);
	}

	APawn* MyOwner = Cast<APawn>(GetOwner());
	if (MyOwner) {
		APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
		if (PC) {
			PC->ClientPlayCameraShake(FireCameraShake);
		}
	}
}

void ASWeapon::PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint) {

	UParticleSystem* SelectedImpact = nullptr;

	switch (SurfaceType) {
	case SURFACE_DEFAULT:
	case SURFACE_FLESHVULNERABLE:
		SelectedImpact = FleshImpactEffect;
		break;
	default:
		SelectedImpact = DefaultImpactEffect;
		break;
	}

	if (SelectedImpact) {
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocket);
		FVector ShotDirection = ImpactPoint - MuzzleLocation;
		ShotDirection.Normalize();
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedImpact, ImpactPoint, ShotDirection.Rotation());
	}
}

void ASWeapon::ServerFire_Implementation() {
	Fire();
}

bool ASWeapon::ServerFire_Validate() {
	return true;
}

void ASWeapon::OnRep_HitScanTrace() {
	// Play Cosmetic effects
	PlayFireEffects(HitScanTrace.TraceTo);
	PlayImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}

FAmmunition ASWeapon::GetAmmunitionInfo() const {
	return Ammunition;
}

void ASWeapon::RefilAmmo(bool bIsMaxAmmo) {
	if (Ammunition.MaxAmmo != DefaultMaxAmmo) {
		if (bIsMaxAmmo) {
			Ammunition.MaxAmmo = DefaultMaxAmmo;
			Ammunition.CurrentAmmo = Ammunition.FullClip;
		} else {
			Ammunition.CurrentAmmo = Ammunition.FullClip;
			if (Ammunition.MaxAmmo + (2 * Ammunition.FullClip) <= DefaultMaxAmmo) {
				Ammunition.MaxAmmo += 2 * Ammunition.FullClip;
			} else {
				Ammunition.MaxAmmo = DefaultMaxAmmo;
			}
		}
	} 
}

void ASWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ASWeapon, HitScanTrace, COND_SkipOwner);
	DOREPLIFETIME(ASWeapon, Ammunition);
}
