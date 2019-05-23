// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HeroController.h"
#include "HeroCharacter.h"

#include "DeathmatchGameMode.generated.h"

/**
 * 
 */
UCLASS()
class MEATREALM_API ADeathmatchGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADeathmatchGameMode();
	//void BeginPlay() override;
	void PostLogin(APlayerController* NewPlayer) override;
	void Logout(AController* Exiting) override;


private:
	TArray<AHeroController*> ConnectedHeroControllers;

	void BindEvents(AHeroController* Controller);
	void UnbindEvents(AHeroController* c);

	void OnPlayerDie(AHeroCharacter* dead, AHeroCharacter* killer);
};
