// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "ItemArmour.h"


#include "InventoryComp.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogInventory, Display, All);

struct FWeaponConfig;
class AItemBase;
class AWeapon;
class AHeroCharacter;
class AEquippableBase;

class IInventoryCompDelegate
{
public:
	virtual ~IInventoryCompDelegate() = default;
	virtual uint32 GetControllerId() = 0;
	virtual FTransform GetHandSocketTransform() = 0;
	virtual void RefreshWeaponAttachments() = 0;
};


UENUM(BlueprintType)
enum class EInventorySlots : uint8
{
	Undefined = 0,
	Primary = 1,
	Secondary = 2,
	Health = 3,
	Armour = 4,
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MEATREALM_API UInventoryComp : public UActorComponent
{
	GENERATED_BODY()

// Public Data ////////////////////////////////////////////////////////////////
public:
	
// Protected Data /////////////////////////////////////////////////////////////
protected:
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
		TArray<TSubclassOf<class AWeapon>> DefaultWeaponClass;
	
	UPROPERTY(Replicated, BlueprintReadOnly)
		EInventorySlots CurrentInventorySlot = EInventorySlots::Undefined;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int32 HealthSlotLimit = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int32 ArmourSlotLimit = 6;

// Private Data ///////////////////////////////////////////////////////////////
private:
	IInventoryCompDelegate* Delegate = nullptr;
	
	UPROPERTY(Replicated)
		EInventorySlots LastInventorySlot = EInventorySlots::Undefined;
	UPROPERTY(Replicated)
		AWeapon* PrimaryWeaponSlot = nullptr;
	UPROPERTY(Replicated)
		AWeapon* SecondaryWeaponSlot = nullptr;
	UPROPERTY(Replicated)
		TArray<AItemBase*> HealthSlot{};
	UPROPERTY(Replicated)
		TArray<AItemBase*> ArmourSlot{};

	FTimerHandle EquipTimerHandle;
	bool bIsEquipping = false;;
	bool bInventoryDestroyed = false;

	
	const char* HandSocketName = "HandSocket"; // TODO remove this duplicated code from AHeroCharacter

	
// Public Methods /////////////////////////////////////////////////////////////
public:	
	UInventoryComp();
	
	void InitInventory();
	void DestroyInventory();

	void NotifyItemIsExpended(AItemBase* Item);

	UFUNCTION(BlueprintCallable)
		AEquippableBase* GetEquippable(EInventorySlots Slot) const;
	UFUNCTION(BlueprintCallable)
		AWeapon* GetWeapon(EInventorySlots Slot) const;
	UFUNCTION(BlueprintCallable)
		AItemBase* GetItem(EInventorySlots Slot) const;
	

	UFUNCTION(BlueprintCallable)
		int GetHealthItemCount() const;
	UFUNCTION(BlueprintCallable)
		int GetArmourItemCount() const;

	UFUNCTION(BlueprintCallable)
		AEquippableBase* GetCurrentEquippable() const;
	UFUNCTION(BlueprintCallable)
		AWeapon* GetCurrentWeapon() const;
	UFUNCTION(BlueprintCallable)
		AItemBase* GetCurrentItem() const;

	UFUNCTION(BlueprintCallable)
		int GetHealthSlotLimit() const { return HealthSlotLimit; }
	
	UFUNCTION(BlueprintCallable)
		int GetArmourSlotLimit() const { return ArmourSlotLimit; }

	UFUNCTION(BlueprintCallable)
	EInventorySlots GetCurrentInventorySlot() const { return CurrentInventorySlot; }
	
	EInventorySlots GetLastInventorySlot() const { return LastInventorySlot; }
	
	void SpawnHeldWeaponsAsPickups() const;
	
	AWeapon* FindWeaponToReceiveAmmo() const;

	void EquipSlot(EInventorySlots Slot);
	bool HasAnItemEquipped() const;
	bool HasAWeaponEquipped() const;
	
	void GiveItemToPlayer(TSubclassOf<class AItemBase> ItemClass);
	void GiveWeaponToPlayer(TSubclassOf<class AWeapon> WeaponClass, FWeaponConfig& Config);

	EInventorySlots FindGoodWeaponSlot() const;

	bool CanGiveItem(const TSubclassOf<AItemBase>& Class);
	void SetDelegate(IInventoryCompDelegate* Dgate) { Delegate = Dgate; }
	bool RemoveEquippableFromInventory(AEquippableBase* Equippable);


// Protected Methods //////////////////////////////////////////////////////////
protected:

// Private Methods ////////////////////////////////////////////////////////////
private:
	AItemBase* GetFirstHealthItemOrNull() const;
	AItemBase* GetFirstArmourItemOrNull() const;
	
	AWeapon* AuthSpawnWeapon(TSubclassOf<AWeapon> weaponClass, FWeaponConfig& Config);
	AWeapon* AssignWeaponToInventorySlot(AWeapon* Weapon, EInventorySlots Slot);
	void MakeEquippedItemVisible() const;
	void SpawnWeaponPickups(TArray<AWeapon*>& Weapons) const;

	bool HasAuthority() const { return GetOwnerRole() == ROLE_Authority; }
};
