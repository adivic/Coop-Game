// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SGrenadeActor.generated.h"

UCLASS()
class COOPGAME_API ASGrenadeActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASGrenadeActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component)
	UStaticMeshComponent* MeshComp;

	UPROPERTY(EditDefaultsOnly, Category = Grenade)
	float DamageRadius;

	UPROPERTY(EditDefaultsOnly, Category = Grenade)
	class UParticleSystem* ExplosionEffect;

	UPROPERTY(EditDefaultsOnly, Category = Grenade)
	class USoundCue* ExplosionSound;

	UPROPERTY(EditDefaultsOnly, Category = Grenade)
	TSubclassOf<UDamageType> DamageType;

	UFUNCTION(Server, Reliable)
	void Explode();

	UPROPERTY(ReplicatedUsing = OnRep_Exploded)
	bool bExploded;

	UFUNCTION()
	void Exploding();

	UFUNCTION()
	void OnRep_Exploded();
};
