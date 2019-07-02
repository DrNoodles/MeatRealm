// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MeatyCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class MEATREALM_API UMeatyCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	float GetMaxSpeed() const override;
};
