// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "SThrowableComponent.generated.h"

USTRUCT(BlueprintType) 
struct FThrowParameters {
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	float Distance;

	UPROPERTY(BlueprintReadOnly)
	FVector_NetQuantize TossVelocity;

	UPROPERTY(BlueprintReadOnly)
	FPredictProjectilePathResult PredictResult;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COOPGAME_API USThrowableComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USThrowableComponent();

protected:

	UPROPERTY(EditDefaultsOnly, Category = Grenade)
	float ThrowVelocity = 1500.f;

	UPROPERTY(EditDefaultsOnly, Category = Grenade)
	TSubclassOf<AActor> Tracer;

	UFUNCTION(Client, Reliable)
	void DrawTracePath();

public:	
	
	void CalculatePath();

	UFUNCTION(Server, Reliable)
	void GetSuggestedToss();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Grenade)
	FThrowParameters ThrowParams;
};
