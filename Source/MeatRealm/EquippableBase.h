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

UENUM()
enum class EEquipState : uint8
{
	Undefined = 0, UnEquipped, Equipping, Equipped, UnEquipping,
};

UCLASS(Abstract)
class MEATREALM_API AEquippableBase : public AActor
{
	GENERATED_BODY()


		// Public Data ////////////////////////////////////////////////////////////////
public:


	// Protected Data /////////////////////////////////////////////////////////////
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float EquipDuration = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float UnEquipDuration = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FString EquippableName = "NoNameThingy";

	UInventoryComp* Delegate = nullptr;


	// Private Data ///////////////////////////////////////////////////////////////
private:
	FTimerHandle EquipTimerHandle;
	FTimerHandle UnEquipTimerHandle;

	UPROPERTY(Replicated) // TODO OnRep_ func. This func could broadcast Blueprint/Event's?
	EEquipState EquippedStatus = EEquipState::Undefined;



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
	virtual EInventoryCategory GetInventoryCategory() { unimplemented(); return EInventoryCategory::Undefined; }
	virtual bool ShouldHideWhenUnequipped() { unimplemented(); return false; }

	void SetDelegate(UInventoryComp* NewDelegate)
	{
		check(HasAuthority())
		Delegate = NewDelegate;
	}

	float GetEquipDuration() const { return EquipDuration; }
	float GetUnEquipDuration() const { return UnEquipDuration; }

	bool Is(EInventoryCategory Category);

	EEquipState GetEquippedStatus() const { return EquippedStatus; }
	bool IsEquipping() const { return EquippedStatus == EEquipState::Equipping; }
	bool IsEquipped() const { return EquippedStatus == EEquipState::Equipped; }
	bool IsUnEquipping() const { return EquippedStatus == EEquipState::UnEquipping; }
	bool IsUnEquipped() const { return EquippedStatus == EEquipState::UnEquipped; }

	FString GetEquippableName() const { return EquippableName; }

	
// Protected Methods //////////////////////////////////////////////////////////
protected:

	
// Private Methods ////////////////////////////////////////////////////////////
private:
	void EquipFinish();
	void UnEquipFinish();

	void ClearTimers();
};
