// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"

#include "DeathmatchGameState.generated.h"


class UScoreboardEntryData;

UCLASS()
class MEATREALM_API ADeathmatchGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	TArray<UScoreboardEntryData*> GetScoreboard();
};

