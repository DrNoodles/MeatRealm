// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PickupBase.h"

#include "WeaponPickupBase.generated.h"

/**
 * 
 */
UCLASS()
class MEATREALM_API AWeaponPickupBase : public APickupBase
{
	GENERATED_BODY()


protected:
	bool TryApplyAffect(IAffectableInterface* const Affectable) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = Pickup)
		TSubclassOf<class AWeapon> WeaponClass;
};
