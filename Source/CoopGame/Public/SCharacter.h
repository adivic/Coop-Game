// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class ASWeapon;
class USHealthComponent;

USTRUCT(BlueprintType)
struct FCPlayerInfo {
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText PlayerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* PlayerImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ACharacter> PlayerCharacter;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* PlayerCharacterImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText PlayerStatus;
};


UCLASS()
class COOPGAME_API ASCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void MoveForward(float value);

	void MoveRight(float value);

	void BeginCrouch();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
	UCameraComponent* PlayerCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
	USHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Components)
	class USThrowableComponent* ThrowableComponent;

	UPROPERTY(EditDefaultsOnly, Category = Player)
	TSubclassOf<AActor> BodyPart;

	UPROPERTY(EditDefaultsOnly, Category = Player, meta= (ClampMin = 0.1, ClampMax = 100))
	float ZoomInterpSpeed;

	UPROPERTY(EditDefaultsOnly, Category = Player)
	float ZoomedFOV;

	float DefaultFOV;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Player)
	bool bLobbyPose;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartADS();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStopADS();

	UPROPERTY(Replicated, BlueprintReadOnly)
	ASWeapon* CurrentWeapon;

	UPROPERTY(EditDefaultsOnly, Category = Player)
	TSubclassOf<class ASWeapon> StarterWeaponClass;

	UPROPERTY(VisibleDefaultsOnly, Category = Player)
	FName WeaponAttachSocket;

	UFUNCTION()
	void OnHealthChanged(USHealthComponent* HealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	/*Pawn died*/
	UPROPERTY(Replicated , BlueprintReadOnly, Category = Player)
	bool bDied = false;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = Player)
	float AimPitch;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartSprint();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStopSprint();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Player)
	bool bIsSprint;

	float DefaultWalkSpeed;

	UPROPERTY(ReplicatedUsing=OnRep_SetupRagdoll, BlueprintReadOnly, Category = Player)
	bool bIsRagdoll;

	UFUNCTION()
	void OnRep_SetupRagdoll();

	UFUNCTION()
	void Reload();

	UPROPERTY(Replicated, EditDefaultsOnly, Category = Player)
	class UAnimMontage* ReloadAnim;

	UPROPERTY(Replicated, EditDefaultsOnly, Category = Player)
	UAnimMontage* VaultAnim;

	UPROPERTY(EditDefaultsOnly, Category = Player)
	UAnimMontage* ClimbAnim;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayMontage(UAnimMontage* MontageToPlay);

	UFUNCTION(Server, Reliable)
	void ServerPlayReloadMontage();

	UFUNCTION(Server, Reliable)
	void Pickup();

	UPROPERTY(EditDefaultsOnly, Category = Sound)
	class USoundCue* NegativeSound;

	UPROPERTY(EditDefaultsOnly, Category = Sound)
	class USoundCue* PickupSound;

	virtual void Jump() override;

	UFUNCTION(Server, Reliable)
	void Vault();

	bool CanVault() const;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = Health)
	void OnDeath(AActor* DamageCauser, ASWeapon* EnemyWeapon);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual FVector GetPawnViewLocation() const override;

	UFUNCTION(BlueprintCallable, Category = Player)
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = Player)
	void StopFire();

	UPROPERTY(Replicated, BlueprintReadWrite, Category = Player)
	bool bWantsToZoom = false;

	UFUNCTION(BlueprintCallable, Category = Player)
	class ASWeapon* GetWeapon() const;

	UPROPERTY()
	bool bFPressed;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = Player)
	FCPlayerInfo PlayerInfo;
};
