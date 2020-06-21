// Fill out your copyright notice in the Description page of Project Settings.


#include "CoopGame/Public/Components/SThrowableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "SCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Components/SplineComponent.h"

// Sets default values for this component's properties
USThrowableComponent::USThrowableComponent()
{

}

void USThrowableComponent::DrawProjectile() {
	for (auto path : ThrowParams.PredictResult.PathData) {
		DrawDebugCircle(GetWorld(), path.Location, 5, 10, FColor::Red, false, 0);
	}
}

void USThrowableComponent::CalculatePath() {
	//Randomize this, change ASCharacter with Pawn
	ASCharacter* MyPawn = Cast<ASCharacter>(GetOwner());

	if (MyPawn) {
		TArray<AActor*> IgnoreActors;
		IgnoreActors.Add(MyPawn);
		FPredictProjectilePathParams PathParams;
		PathParams.LaunchVelocity = FVector(MyPawn->GetControlRotation().Vector() * ThrowVelocity);
		PathParams.DrawDebugTime = 5.f;
		PathParams.MaxSimTime = 10;
		PathParams.StartLocation = MyPawn->GetActorLocation();
		PathParams.ProjectileRadius = 20.f;
		PathParams.TraceChannel = ECC_Visibility;
		PathParams.bTraceWithCollision = true;
		PathParams.ActorsToIgnore = IgnoreActors;

		UGameplayStatics::PredictProjectilePath(GetWorld(), PathParams, ThrowParams.PredictResult);
	
		//Calculate Distance
		ThrowParams.Distance = FVector::Distance(MyPawn->GetActorLocation(), ThrowParams.PredictResult.LastTraceDestination.Location);

		if (MyPawn->IsLocallyControlled()) {
			for (auto path : ThrowParams.PredictResult.PathData) {
				DrawDebugCircle(GetWorld(), path.Location, 5, 10, FColor::Red, false, 0);
			}
		}
	}
}

void USThrowableComponent::GetSuggestedToss_Implementation() {
	ASCharacter* MyPawn = Cast<ASCharacter>(GetOwner());

	if (MyPawn) {
		TArray<AActor*> IgnoreActors;
		IgnoreActors.Add(MyPawn);

		UGameplayStatics::SuggestProjectileVelocity(GetWorld(), ThrowParams.TossVelocity, MyPawn->GetActorLocation(),
			ThrowParams.PredictResult.LastTraceDestination.Location, ThrowVelocity, false, 20.f, 0.f,
			ESuggestProjVelocityTraceOption::DoNotTrace, FCollisionResponseParams::DefaultResponseParam, IgnoreActors, false);
	}
}

void USThrowableComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USThrowableComponent, ThrowParams);
}
