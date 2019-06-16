// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "HeroState.generated.h"

/**
 * 
 */
UCLASS()
class MEATREALM_API AHeroState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Replicated)
		int Kills = 0;

	UPROPERTY(BlueprintReadOnly, Replicated)
		int Deaths = 0;

	// bIsInactive never updates as it's set to only replicate at the beginning of a game. This is stupid and makes the variable 100% useless to clients. Hence, we have our one!
	UPROPERTY(Replicated)
	bool HasLeftTheGame = false;
};
