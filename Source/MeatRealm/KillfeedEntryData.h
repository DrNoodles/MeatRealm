// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "KillfeedEntryData.generated.h"

UCLASS(BlueprintType)
class MEATREALM_API UKillfeedEntryData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		FString Winner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		FString Verb;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		FString Loser;
};
