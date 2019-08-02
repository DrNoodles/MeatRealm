// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PickupBase.h"

#include "ChestPickup.generated.h"

/**
 * 
 */
UCLASS() // TODO rename to APickupSpawnerPickup
class MEATREALM_API AChestPickup : public APickupBase
{
	GENERATED_BODY()
	
	public:
		AChestPickup()
		{
			bExplicitInteraction = true;
			bIsSingleUse = true;
		}

	protected:

		bool TryApplyAffect(IAffectableInterface * const Affectable) override;

	private:
		UPROPERTY(EditDefaultsOnly, Category = Pickup)
			TArray<TSubclassOf<class APickupBase>> PickupClasses;

};
