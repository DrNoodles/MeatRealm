// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"

#include "DeathmatchGameMode.generated.h"

struct FMRHitResult;
struct FActorSpawnParameters;
class AHeroCharacter;
class AHeroController;
class AProjectile;


UCLASS()
class MEATREALM_API ADeathmatchGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ADeathmatchGameMode();

	// Game Lifecycle
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual bool ReadyToEndMatch_Implementation() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
	void OnRestartGame();

	virtual void SetPlayerDefaults(APawn* PlayerPawn) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

	void SpawnAChest() const;

	void PostLogin(APlayerController* NewPlayer) override;
	void Logout(AController* Exiting) override;
	bool ShouldSpawnAtStartSpot(AController* Player) override;
	AActor* FindFurthestPlayerStart(AController* Controller);
	void OnPlayerTakeDamage(FMRHitResult Hit);


private:
	UPROPERTY(EditAnywhere)
		float PowerUpInitialDelay = 30;

	UPROPERTY(EditAnywhere)
		float PowerUpSpawnRate = 120;

	TMap<uint32, AHeroController*> ConnectedHeroControllers;
	TMap<uint32, int> PlayerMappedTints;
	TArray<FColor> PlayerTints;
	int TintCount = 0;

	FTimerHandle ChestSpawnTimerHandle;

	//bool HasMetGameEndConditions() const;
	void AddKillfeedEntry(AHeroController* const Killer, AHeroController* const Dead);
};
