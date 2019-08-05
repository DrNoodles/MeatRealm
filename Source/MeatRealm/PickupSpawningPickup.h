// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PickupBase.h"

#include "PickupSpawningPickup.generated.h"

/**
 * A pickup that when interacted with will spawn a pickup and self destruct
 */
UCLASS()
class MEATREALM_API APickupSpawningPickup : public APickupBase
{
	GENERATED_BODY()
	
	public:
		APickupSpawningPickup()
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
