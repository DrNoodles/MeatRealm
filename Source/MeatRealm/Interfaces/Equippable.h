// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "Equippable.generated.h"

class AHeroCharacter;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UEquippable : public UInterface
{
	GENERATED_BODY()
};


UENUM()
enum class EInventoryCategory : uint8
{
	Undefined = 0, Weapon, Health, Armour, Throwable
};


class MEATREALM_API IEquippable
{
	GENERATED_BODY()

public:
	UFUNCTION()
		virtual void Equip() = 0;
	UFUNCTION()
		virtual void Unequip() = 0;
	UFUNCTION()
		virtual float GetEquipDuration() = 0;
	UFUNCTION()
		virtual void SetHidden(bool bIsHidden) = 0;
	UFUNCTION()
		virtual EInventoryCategory GetInventoryCategory() = 0;
	UFUNCTION()
		virtual void EnterInventory() = 0;
	UFUNCTION()
		virtual void ExitInventory() = 0;
	UFUNCTION()
		virtual bool ShouldHideWhenUnequipped() = 0;
	UFUNCTION()
		virtual void SetDelegate(AHeroCharacter* Delegate) = 0; // TODO Split out an inventory component in HeroCharacter and pass that in here instead as the delegate. UInventoryComponent. Mmmmmm hawt

	// Below have Default implmenetations

	UFUNCTION()
		virtual bool Is(EInventoryCategory Category);
};
