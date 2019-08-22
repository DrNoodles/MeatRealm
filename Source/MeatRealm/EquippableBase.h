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

	
// Public Data ////////////////////////////////////////////////////////////////
public:
	
// Protected Data /////////////////////////////////////////////////////////////
protected: 
	UInventoryComp* Delegate = nullptr;

	
// Private Data ///////////////////////////////////////////////////////////////
private: 

	
// Public Methods /////////////////////////////////////////////////////////////
public:
	AEquippableBase();

	void Equip();
	void Unequip();

	virtual void OnEquipStarted() { unimplemented(); }
	virtual void OnEquipFinished() { unimplemented(); }


	virtual void OnUnEquipStarted() { unimplemented(); }
	virtual void OnUnEquipFinished() { unimplemented(); }
	
	virtual void EnterInventory() { unimplemented(); }
	virtual void ExitInventory() { unimplemented(); }

	// TODO Just make these properties of the base class for the children to set
	virtual float GetEquipDuration() { unimplemented(); return 0; }
	virtual EInventoryCategory GetInventoryCategory() { unimplemented(); return EInventoryCategory::Undefined; }
	virtual bool ShouldHideWhenUnequipped() { unimplemented(); return false; }
	
	void SetDelegate(UInventoryComp* NewDelegate)
	{
		check(HasAuthority())
		Delegate = NewDelegate;
	}
	bool Is(EInventoryCategory Category);

	
// Protected Methods //////////////////////////////////////////////////////////
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	
// Private Methods ////////////////////////////////////////////////////////////
private:

};
