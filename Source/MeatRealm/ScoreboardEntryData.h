// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "ScoreboardEntryData.generated.h"

UCLASS(BlueprintType)
class MEATREALM_API UScoreboardEntryData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		FString Name;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		int Kills;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		int Deaths;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		int Ping;
};
