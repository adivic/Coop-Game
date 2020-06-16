// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SCollectable.generated.h"

UENUM()
enum class ECollectableType : uint8 {
	AMMO, HELATH, MAX_AMMO,
};

UCLASS()
class COOPGAME_API ASCollectable : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASCollectable();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
	class USphereComponent* SphereComp;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(Replicated)
	bool bIsOverlap = false;

	UPROPERTY(EditDefaultsOnly, Category = Collectable)
	ECollectableType Type; 

public:	
	FORCEINLINE bool GetIsOverlap() const { return bIsOverlap; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Collectable)
	FORCEINLINE ECollectableType GetType() const { return Type; }

	UFUNCTION(BlueprintSetter, BlueprintCallable, Category = Collectable)
	void SetType(ECollectableType NewType);
};
