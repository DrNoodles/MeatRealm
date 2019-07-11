// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemBase.h"
#include "ItemHealth.generated.h"

/**
 * 
 */
UCLASS()
class MEATREALM_API AItemHealth : public AItemBase
{
	GENERATED_BODY()

private:
	void ApplyItem(IAffectableInterface* const Affectable) override;
};
