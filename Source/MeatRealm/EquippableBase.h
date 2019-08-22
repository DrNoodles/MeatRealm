// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "EquippableBase.generated.h"

class UInventoryComp;

UENUM()
enum class EInventoryCategory : uint8
{
	Undefined = 0, Weapon, Health, Armour, Throwable
};

UCLASS(Abstract)
class MEATREALM_API AEquippableBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AEquippableBase();

	
	UFUNCTION()
		virtual void Equip() { unimplemented(); }
	UFUNCTION()
		virtual void Unequip() { unimplemented(); }
	UFUNCTION()
		virtual float GetEquipDuration() { unimplemented(); return 0; }
	UFUNCTION()
		virtual void SetHidden(bool bIsHidden) { unimplemented(); }
	UFUNCTION()
		virtual EInventoryCategory GetInventoryCategory() { unimplemented(); return EInventoryCategory::Undefined;
	}
	UFUNCTION()
		virtual void EnterInventory() { unimplemented(); }
	UFUNCTION()
		virtual void ExitInventory() { unimplemented(); }
	UFUNCTION()
		virtual bool ShouldHideWhenUnequipped() { unimplemented(); return false; }
	UFUNCTION()
		virtual void SetDelegate(UInventoryComp* Delegate) { unimplemented(); }

	// Below have Default implementations

	UFUNCTION()
		bool Is(EInventoryCategory Category);

	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
private:

};
