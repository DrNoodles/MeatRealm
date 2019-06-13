// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Weapon.h"
#include "AffectableInterface.generated.h"

class AHeroController;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UAffectableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MEATREALM_API IAffectableInterface
{
	GENERATED_BODY()

public: // NOTE This interface breaks the I in SOLID. But it'll do for now.
	UFUNCTION()
		virtual void AuthApplyDamage(uint32 InstigatorHeroControllerId, float Delta, FVector Location) = 0;
	UFUNCTION()
		virtual bool AuthTryGiveHealth(float Hp) = 0;
	UFUNCTION()
		virtual bool AuthTryGiveArmour(float Delta) = 0;
	UFUNCTION()
		virtual bool AuthTryGiveAmmo() = 0;
	UFUNCTION()
		virtual bool AuthTryGiveWeapon(const TSubclassOf<AWeapon>& Class) = 0;
};
