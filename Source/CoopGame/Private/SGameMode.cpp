// Fill out your copyright notice in the Description page of Project Settings.


#include "SGameMode.h"
#include "TimerManager.h"
#include "SGameState.h"
#include "CoopGame/Public/Components/SHealthComponent.h"
#include "SPlayerState.h"

ASGameMode::ASGameMode() {
	TimerBetweenWaves = 2.f;

	GameStateClass = ASGameMode::StaticClass();
	PlayerStateClass = ASPlayerState::StaticClass();

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.f;
}

void ASGameMode::StartWave() {
	WaveCount++;
	NumOfBotsToSpawn = 2 * WaveCount;

	GetWorldTimerManager().SetTimer(TimerHandle_BotSpawner, this, &ASGameMode::SpawnBotTimerElapsed, 1.f, true, 0);

	SetWaveState(EWaveState::WaveInProgress);
}

void ASGameMode::SpawnBotTimerElapsed() {
	SpawnNewBot();
	NumOfBotsToSpawn--;
	if (NumOfBotsToSpawn <= 0) {
		EndWave();
	}
}

void ASGameMode::StartPlay() {
	Super::StartPlay();

	PrepareForNextWave();
}

void ASGameMode::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);

	CheckWaveState();

	CheckAnyPlayerAlive();
}

void ASGameMode::PrepareForNextWave() {
	
	GetWorldTimerManager().SetTimer(TimerHandle_NextWaveStart, this, &ASGameMode::StartWave, TimerBetweenWaves, false);

	SetWaveState(EWaveState::WaitingToStart);

	RespawnDeadPlayers();
}

void ASGameMode::CheckWaveState() {

	bool bIsPreparingForWave = GetWorldTimerManager().IsTimerActive(TimerHandle_NextWaveStart);

	if (NumOfBotsToSpawn > 0 || bIsPreparingForWave) return;

	bool bIsAnyBotAlive = false;

	for (FConstPawnIterator it = GetWorld()->GetPawnIterator(); it; ++it) {
		APawn* TestPawn = it->Get();
		if (TestPawn == nullptr || TestPawn->IsPlayerControlled()) {
			continue;
		}
		USHealthComponent* HealthComp = Cast<USHealthComponent>(TestPawn->GetComponentByClass(USHealthComponent::StaticClass()));
		if (HealthComp && HealthComp->GetHealth() > 0.0f) {
			bIsAnyBotAlive = true;
			break;
		}
	}
	if (!bIsAnyBotAlive) {
		PrepareForNextWave();

		SetWaveState(EWaveState::WaveComplete);
	}

}

void ASGameMode::CheckAnyPlayerAlive() {
	for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it) {
		APlayerController* PC = it->Get();
		if (PC && PC->GetPawn()) {
			APawn* MyPawn = PC->GetPawn();
			
			USHealthComponent* HealthComp = Cast<USHealthComponent>(MyPawn->GetComponentByClass(USHealthComponent::StaticClass()));
			if (ensure(HealthComp) && HealthComp->GetHealth() > 0.0f) {
				return;
			}
		}
	}
	//No player alive
	GameOver();
}

void ASGameMode::GameOver() {
	EndWave();
	SetWaveState(EWaveState::GameOver);
	for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it) {
		APlayerController* PC = it->Get();
		if (PC) {
			FTimerDelegate Del = FTimerDelegate::CreateLambda([PC]() {PC->ConsoleCommand("quit"); });
			FTimerHandle Handle;
			GetWorldTimerManager().SetTimer(Handle, Del, 3.f, false);
		}
	}
}

void ASGameMode::SetWaveState(EWaveState NewState) {
	ASGameState* GS = GetGameState<ASGameState>();
	if (ensureAlways(GS)) {
		GS->SetWaveState(NewState);
	}
}

void ASGameMode::RespawnDeadPlayers() {
	for (FConstPlayerControllerIterator it = GetWorld()->GetPlayerControllerIterator(); it; ++it) {
		APlayerController* PC = it->Get();
		if (PC && PC->GetPawn() == nullptr) {
			RestartPlayer(PC);
		}
	}
}

void ASGameMode::EndWave() {
	GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawner);
	SetWaveState(EWaveState::WaitingToComplete);
}
