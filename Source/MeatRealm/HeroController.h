// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HeroController.generated.h"

class AHeroCharacter;

UCLASS()
class MEATREALM_API AHeroController : public APlayerController
{
	GENERATED_BODY()

public:
	AHeroCharacter* GetHeroCharacter() const;
};
