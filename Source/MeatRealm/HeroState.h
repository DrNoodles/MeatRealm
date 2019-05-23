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
	UPROPERTY(BlueprintReadOnly)
		int Kills = 2;

	UPROPERTY(BlueprintReadOnly)
		int Deaths = 3;
};
