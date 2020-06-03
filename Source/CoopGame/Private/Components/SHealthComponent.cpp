// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SHealthComponent.h"
#include "..\..\Public\Components\SHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "SGameMode.h"

// Sets default values for this component's properties
USHealthComponent::USHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	DefaultHealth = 100;
	bIsDead = false; 
	TeamNum = 255;

	SetIsReplicated(true);
}


// Called when the game starts
void USHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwnerRole() == ROLE_Authority) {
		AActor* MyOwner = GetOwner();
		if (MyOwner) {
			MyOwner->OnTakeAnyDamage.AddDynamic(this, &USHealthComponent::HandleTakeAnyDamage);
		}
	}
	
	Health = DefaultHealth;
}

void USHealthComponent::OnRep_Healht(float OldHealth) {

	float Damage = Health - OldHealth;
	OnHealthChanged.Broadcast(this, Health, Damage, nullptr, nullptr, nullptr);
}

void USHealthComponent::HandleTakeAnyDamage(AActor * DamagedActor, float Damage, const class UDamageType * DamageType, class AController* InstigatedBy, AActor * DamageCauser) {
	if (Damage <= 0 || bIsDead) return;

	if (DamageCauser != DamagedActor && IsFriendly(DamagedActor, DamageCauser)) return;

	Health = FMath::Clamp(Health - Damage, 0.0f, DefaultHealth);
	//UE_LOG(LogTemp, Log, TEXT("Health Changed: %f"), Health);

	bIsDead = Health <= 0.0f;

	OnHealthChanged.Broadcast(this, Health, Damage, DamageType, InstigatedBy, DamageCauser);

	if (bIsDead) {
		ASGameMode* GM = Cast<ASGameMode>(GetWorld()->GetAuthGameMode());
		if (GM) {
			GM->OnActorKilled.Broadcast(GetOwner(), DamageCauser, InstigatedBy);
		}
	}
}

float USHealthComponent::GetHealth() const {
	return Health;
}

void USHealthComponent::Heal(float HealAmout) {
	
	if (HealAmout <= 0.0f || Health <= 0.0f) return;

	Health = FMath::Clamp(Health + HealAmout, 0.0f, DefaultHealth);
	
	UE_LOG(LogTemp, Warning, TEXT("Health Changed: %f (+%s)"), Health, *FString::SanitizeFloat(HealAmout));

	OnHealthChanged.Broadcast(this, Health, -HealAmout, nullptr, nullptr, nullptr);
}

bool USHealthComponent::IsFriendly(AActor* ActorA, AActor* ActorB) {
	if (ActorA == nullptr || ActorB == nullptr) {
		return false;
	}

	USHealthComponent* HealthCompA = Cast<USHealthComponent>(ActorA->GetComponentByClass(USHealthComponent::StaticClass()));
	USHealthComponent* HealthCompB = Cast<USHealthComponent>(ActorB->GetComponentByClass(USHealthComponent::StaticClass()));

	if (HealthCompA == nullptr || HealthCompB == nullptr) {
		//Assume firednly
		return true;
	}

	return HealthCompA->TeamNum == HealthCompB->TeamNum;
}

void USHealthComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USHealthComponent, Health);
}

