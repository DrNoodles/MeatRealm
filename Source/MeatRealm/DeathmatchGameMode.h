// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PickupSpawnLocation.h"

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

	void AnnounceChestSpawn();
	void SpawnChest();

	void PostLogin(APlayerController* NewPlayer) override;
	void Logout(AController* Exiting) override;
	bool ShouldSpawnAtStartSpot(AController* Player) override;
	AActor* FindFurthestPlayerStart(AController* Controller);
	void OnPlayerTakeDamage(FMRHitResult Hit);

private:

	// Time before initial announcement
	UPROPERTY(EditAnywhere)
		float PowerUpInitialDelay = 30;

	// Time after announcement till the item spawns
	UPROPERTY(EditAnywhere)
		float PowerUpAnnouncementLeadTime = 15;

	UPROPERTY(EditAnywhere)
		float PowerUpSpawnRate = 120;

	TMap<uint32, AHeroController*> ConnectedHeroControllers;
	TMap<uint32, int> PlayerMappedTints;
	TArray<FColor> PlayerTints;
	int TintCount = 0;

	FTimerHandle ChestAnnouncementTimerHandle;
	FTimerHandle ChestSpawnTimerHandle;
	APickupSpawnLocation* NextChestSpawnLocation = nullptr;

	//bool HasMetGameEndConditions() const;
	void AddKillfeedEntry(AHeroController* const Killer, AHeroController* const Dead);
};
