// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Equippable.h"

#include "EquippableBase.generated.h"

UCLASS(Abstract)
class MEATREALM_API AEquippableBase : public AActor, public IEquippable
{
	GENERATED_BODY()
	
public:	
	AEquippableBase();

	
	/* IEquippable */
	UFUNCTION()
		virtual void Equip() override { unimplemented(); }
	UFUNCTION()
		virtual void Unequip() override { unimplemented(); }
	UFUNCTION()
		virtual float GetEquipDuration() override { unimplemented(); return 0; }
	UFUNCTION()
		virtual void SetHidden(bool bIsHidden) override { unimplemented(); }
	UFUNCTION()
		virtual EInventoryCategory GetInventoryCategory() override
	{
		unimplemented(); return EInventoryCategory::Undefined;
	}
	UFUNCTION()
		virtual void EnterInventory() override { unimplemented(); }
	UFUNCTION()
		virtual void ExitInventory()  override { unimplemented(); }
	UFUNCTION()
		virtual bool ShouldHideWhenUnequipped() override { unimplemented(); return false; }
	UFUNCTION()
		virtual void SetDelegate(UInventoryComp* Delegate) override { unimplemented(); }

	// Below have Default implementations

	UFUNCTION()
		virtual bool Is(EInventoryCategory Category) override;
	/* IEquippable End */

	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
private:

};
