// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PickupBase.h"
#include "ItemPickupBase.generated.h"

/**
 * 
 */
UCLASS()
class MEATREALM_API AItemPickupBase : public APickupBase
{
	GENERATED_BODY()

public:
	AItemPickupBase()
	{
		bExplicitInteraction = true;
	}

	bool CanInteract(IAffectableInterface* const Affectable, float& OutDelay) override;

protected:

	bool TryApplyAffect(IAffectableInterface* const Affectable) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = Pickup)
		TSubclassOf<class AItemBase> ItemClass;
};
