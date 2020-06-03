// Fill out your copyright notice in the Description page of Project Settings.


#include "SProjectileWeapon.h"

void ASProjectileWeapon::Fire() {

	AActor* MyOwner = GetOwner();
	if (MyOwner && ProjectileClass) {

		FVector EyeLocation;
		FRotator EyeRotator;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotator);

		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocket);
		FRotator MuzzleRotation = MeshComp->GetSocketRotation(MuzzleSocket);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, MuzzleRotation, SpawnParams);

	}
}
