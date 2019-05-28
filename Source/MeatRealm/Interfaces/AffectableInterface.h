// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AffectableInterface.generated.h"

class AHeroCharacter;
class AWeapon;

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
		virtual void ApplyDamage(AHeroCharacter* DamageInstigator, float Delta) = 0;
	UFUNCTION()
		virtual bool TryGiveHealth(float Hp) = 0;
	UFUNCTION()
		virtual bool TryGiveAmmo(int Ammo) = 0;
	//UFUNCTION()
		//virtual void GiveWeapon(AWeapon* Delta) = 0;
};
