// Fill out your copyright notice in the Description page of Project Settings.


#include "SCollectable.h"
#include "SCharacter.h"
#include "SWeapon.h"
#include "Net/UnrealNetwork.h"
#include "Components/SphereComponent.h"

// Sets default values
ASCollectable::ASCollectable()
{
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	RootComponent = MeshComp;

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(200.f);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECR_Overlap);
	SphereComp->SetupAttachment(MeshComp);

	Type = ECollectableType::AMMO;

	SetReplicates(true);
}

// Called when the game starts or when spawned
void ASCollectable::BeginPlay()
{
	Super::BeginPlay();
	SphereComp->OnComponentBeginOverlap.AddDynamic(this, &ASCollectable::OnBeginOverlap);
	SphereComp->OnComponentEndOverlap.AddDynamic(this, &ASCollectable::OnEndOverlap);
}

void ASCollectable::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult) {
	ASCharacter* OverlappedCharacter = Cast<ASCharacter>(OtherActor);
	if (OverlappedCharacter) {
		bIsOverlap = true;
		OnOverlapBegin(OverlappedCharacter);
	}
}

void ASCollectable::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) {
	ASCharacter* OverlappedCharacter = Cast<ASCharacter>(OtherActor);
	if (OverlappedCharacter) {
		bIsOverlap = false;
		OnOverlapEnd(OverlappedCharacter);
	}
}

void ASCollectable::OnOverlapBegin_Implementation(ASCharacter* PlayerCharacter) {} //Override in BLueprints

void ASCollectable::OnOverlapEnd_Implementation(ASCharacter* PlayerCharacter) {}// Override in Blueprints


void ASCollectable::SetType(ECollectableType NewType) {
	Type = NewType;
}

void ASCollectable::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCollectable, bIsOverlap);
}

