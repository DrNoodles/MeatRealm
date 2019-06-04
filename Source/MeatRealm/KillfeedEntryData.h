// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UnrealNetwork.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Info.h"

#include "KillfeedEntryData.generated.h"


UCLASS(BlueprintType)
class MEATREALM_API UKillfeedEntryData : public UActorComponent
{
	GENERATED_BODY()

public:
	
	virtual bool IsSupportedForNetworking() const override
	{
		return true;
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
		FString Winner = "Winrar";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
		FString Verb = "Blapped";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
		FString Loser = "Ded";
};
