// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LocalPlayer.h"
#include "MRLocalPlayer.generated.h"

/**
 * 
 */
UCLASS()
class MEATREALM_API UMRLocalPlayer : public ULocalPlayer
{
	GENERATED_BODY()

	FString GetNickname() const override;

private:
	FString PlayerName = "FreshMeat";
};
