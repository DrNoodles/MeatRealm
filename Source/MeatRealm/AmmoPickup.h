// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PickupBase.h"

#include "AmmoPickup.generated.h"

/**
 * 
 */
UCLASS()
class MEATREALM_API AAmmoPickup : public APickupBase
{
	GENERATED_BODY()

protected:
	bool TryApplyAffect(IAffectableInterface* const Affectable) override;
};
