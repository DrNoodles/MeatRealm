// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryComp.h"
#include "Engine/World.h"
#include "UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon.h"
#include "WeaponPickupBase.h"
#include "EquippableBase.h"

DEFINE_LOG_CATEGORY(LogInventory);

// Lifecycle //////////////////////////////////////////////////////////////////

UInventoryComp::UInventoryComp()
{
//	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);
}
void UInventoryComp::InitInventory()
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::InitInventory()"));
	
	// Randomly select a weapon
	if (DefaultWeaponClass.Num() > 0)
	{
		if (HasAuthority())
		{
			const auto Choice = FMath::RandRange(0, DefaultWeaponClass.Num() - 1);
			const auto WeaponClass = DefaultWeaponClass[Choice];
			auto Config = FWeaponConfig{};

			GiveWeaponToPlayer(WeaponClass, Config);
		}
	}
}
void UInventoryComp::DestroyInventory()
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::DestroyInventory()"));
	
	if (bInventoryDestroyed)
	{
		//LogMsgWithRole("Attempted to destroy already destoryed inventory!");
		UE_LOG(LogInventory, Warning, TEXT("Attempted to destroy already destoryed inventory!"));
		return;
	}

	LastInventorySlot = EInventorySlots::Undefined;
	CurrentInventorySlot = EInventorySlots::Undefined;
	if (PrimaryWeaponSlot)
	{
		PrimaryWeaponSlot->Destroy();
		PrimaryWeaponSlot = nullptr;
	}
	if (SecondaryWeaponSlot)
	{
		SecondaryWeaponSlot->Destroy();
		SecondaryWeaponSlot = nullptr;
	}


	for (auto* HP : HealthSlot)
	{
		if (HP)
			HP->Destroy();
		else
			UE_LOG(LogInventory, Warning, TEXT("Attempted to destroy HP slot item that's null"));
	}

	for (auto* AP : ArmourSlot)
	{
		if (AP)
			AP->Destroy();
		else
			UE_LOG(LogInventory, Warning, TEXT("Attempted to destroy HP slot item that's null"));
	}
	
	for (auto* Th : ThrowableSlot)
	{
		if (Th)
			Th->Destroy();
		else
			UE_LOG(LogInventory, Warning, TEXT("Attempted to destroy throwable slot item that's null"));
	}
	
	HealthSlot.Empty();
	ArmourSlot.Empty();
	ThrowableSlot.Empty();
	
	bInventoryDestroyed = true;
}


// Replication ////////////////////////////////////////////////////////////////

void UInventoryComp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Just the owner
	DOREPLIFETIME_CONDITION(UInventoryComp, LastInventorySlot, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UInventoryComp, CurrentInventorySlot, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UInventoryComp, PrimaryWeaponSlot, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UInventoryComp, SecondaryWeaponSlot, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UInventoryComp, HealthSlot, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UInventoryComp, ArmourSlot, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UInventoryComp, ThrowableSlot, COND_OwnerOnly);
}


// Queries ////////////////////////////////////////////////////////////////////

AWeapon* UInventoryComp::GetWeapon(EInventorySlots Slot) const
{
	UE_LOG(LogInventory, VeryVerbose, TEXT("UInventoryComp::GetWeapon()"));
	switch (Slot)
	{
	case EInventorySlots::Primary: return PrimaryWeaponSlot;
	case EInventorySlots::Secondary: return SecondaryWeaponSlot;

	default:
		return nullptr;
	}
}
AItemBase* UInventoryComp::GetItem(EInventorySlots Slot) const
{
	UE_LOG(LogInventory, VeryVerbose, TEXT("UInventoryComp::GetItem()"));
	switch (Slot)
	{
	case EInventorySlots::Health: return GetFirstHealthItemOrNull();
	case EInventorySlots::Armour: return GetFirstArmourItemOrNull();
		//case EInventorySlots::Secondary: return SecondaryWeaponSlot;

	default:
		return nullptr;
	}
}
AEquippableBase* UInventoryComp::GetEquippable(EInventorySlots Slot) const
{
	UE_LOG(LogInventory, VeryVerbose, TEXT("UInventoryComp::GetEquippable()"));
	switch (Slot)
	{
	case EInventorySlots::Primary: return PrimaryWeaponSlot;
	case EInventorySlots::Secondary: return SecondaryWeaponSlot;
	case EInventorySlots::Health: return GetFirstHealthItemOrNull();
	case EInventorySlots::Armour: return GetFirstArmourItemOrNull();

	case EInventorySlots::Undefined:
	default:;
	}

	return nullptr;
}

int UInventoryComp::GetHealthItemCount() const
{
	UE_LOG(LogInventory, VeryVerbose, TEXT("UInventoryComp::GetHealthItemCount()"));
	return HealthSlot.Num();
}
int UInventoryComp::GetArmourItemCount() const
{
	UE_LOG(LogInventory, VeryVerbose, TEXT("UInventoryComp::GetArmourItemCount()"));
	return ArmourSlot.Num();
}
int UInventoryComp::GetThrowableItemCount() const
{
	UE_LOG(LogInventory, VeryVerbose, TEXT("UInventoryComp::GetThrowableItemCount()"));
	return ThrowableSlot.Num();
}

AWeapon* UInventoryComp::GetCurrentWeapon() const
{
	UE_LOG(LogInventory, VeryVerbose, TEXT("UInventoryComp::GetCurrentWeapon()"));
	return GetWeapon(CurrentInventorySlot);
}
AItemBase* UInventoryComp::GetCurrentItem() const
{
	UE_LOG(LogInventory, VeryVerbose, TEXT("UInventoryComp::GetCurrentItem()"));
	return GetItem(CurrentInventorySlot);
}
AEquippableBase* UInventoryComp::GetCurrentEquippable() const
{
	UE_LOG(LogInventory, VeryVerbose, TEXT("UInventoryComp::GetCurrentEquippable()"));
	return GetEquippable(CurrentInventorySlot);
}

bool UInventoryComp::HasAnItemEquipped() const
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::HasAnItemEquipped()"));
	auto Equippable = GetEquippable(CurrentInventorySlot);
	if (!Equippable) return false;
	return Equippable->Is(EInventoryCategory::Health) || Equippable->Is(EInventoryCategory::Armour);
}
bool UInventoryComp::HasAWeaponEquipped() const
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::HasAWeaponEquipped()"));
	auto Equippable = GetEquippable(CurrentInventorySlot);
	if (!Equippable) return false;
	return Equippable->Is(EInventoryCategory::Weapon);// || Equippable->Is(EInventoryCategory::Throwable);
}

AWeapon* UInventoryComp::FindWeaponToReceiveAmmo() const
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::FindWeaponToReceiveAmmo()"));
	// Try give ammo to equipped weapon
	AWeapon* CurrentWeapon = GetCurrentWeapon();
	if (CurrentWeapon && CurrentWeapon->CanGiveAmmo())
	{
		return CurrentWeapon; // ammo given to main weapon
	}

	// TODO Give ammo to current weapon, if that fails give it to the other slot. If no weapon is equipped, give it to the first holstered gun found


	// Try give ammo to alternate weapon!
	AWeapon* AltWeap = nullptr;
	if (CurrentInventorySlot == EInventorySlots::Primary) AltWeap = GetWeapon(EInventorySlots::Secondary);
	if (CurrentInventorySlot == EInventorySlots::Secondary) AltWeap = GetWeapon(EInventorySlots::Primary);
	if (AltWeap && AltWeap->CanGiveAmmo() && AltWeap->TryGiveAmmo())
	{
		return AltWeap; // ammo given to alt weapon
	}

	return nullptr;
}


// Spawning ///////////////////////////////////////////////////////////////////

bool UInventoryComp::CanGiveItem(const TSubclassOf<AItemBase>& Class)
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::CanGiveItem()"));

	// Health and Armour items have limits. Check if we're within them
	const int HealthCount = HealthSlot.Num();
	const int ArmourCount = ArmourSlot.Num();

	// If we certainly have space, lets go!
	if (HealthCount < HealthSlotLimit && ArmourCount < ArmourSlotLimit)
		return true;


	// Need to create an instance to see what category it is
	auto Temp = NewObject<AItemBase>(this, Class);
	const auto Category = Temp->GetInventoryCategory();
	Temp->Destroy();

	if (Category == EInventoryCategory::Health && HealthCount == HealthSlotLimit)
		return false;

	if (Category == EInventoryCategory::Armour && ArmourCount == ArmourSlotLimit)
		return false;

	return true;
}
void UInventoryComp::GiveItemToPlayer(TSubclassOf<AItemBase> ItemClass)
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::GiveItemToPlayer()"));

	check(HasAuthority() && GetWorld() && ItemClass)


	// Spawn the item at the hand socket
	const auto TF = Delegate->GetHandSocketTransform();
	AItemBase* Item = GetWorld()->SpawnActorDeferred<AItemBase>(
		ItemClass,
		TF,
		GetOwner(), // owner actor
		(APawn*)GetOwner(), // instigator pawn // TODO Fix this cast to Pawn
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	UGameplayStatics::FinishSpawningActor(Item, TF);

	Item->SetActorHiddenInGame(true);

	// Find correct slot
	auto Slot = EInventorySlots::Undefined;

	switch (Item->GetInventoryCategory()) {
	case EInventoryCategory::Health: Slot = EInventorySlots::Health; break;
	case EInventoryCategory::Armour: Slot = EInventorySlots::Armour; break;
	case EInventoryCategory::Weapon: break;
	case EInventoryCategory::Undefined: break;
	case EInventoryCategory::Throwable: break;
	default:;
	}


	const auto Recipient = Cast<IAffectableInterface>(GetOwner());
	if (!Recipient)
	{
		UE_LOG(LogInventory, Error, TEXT("Failed to cast inventory owner to IAffectableInterface!"));
	}

	// Assign Item to inventory slot - TODO Combine this with the Assign weapon to slot code
	switch (Slot) {

	case EInventorySlots::Health:
	{
		// Add to inv
		HealthSlot.Add(Item);
		Item->EnterInventory();
		Item->SetRecipient(Recipient);
		Item->SetDelegate(this);
	}
	break;

	case EInventorySlots::Armour:
	{
		// Add to inv
		ArmourSlot.Add(Item);
		Item->EnterInventory();
		Item->SetRecipient(Recipient); 
		Item->SetDelegate(this);
	}
	break;

	case EInventorySlots::Undefined: break;
	case EInventorySlots::Primary: break;
	case EInventorySlots::Secondary: break;
	case EInventorySlots::Throwable: break;
	default:;
	}


	// Put it in our hands! TODO - Or not?
	//EquipSlot(Slot);

	//LogMsgWithRole("UInventoryComp::GiveItemToPlayer2");
}
void UInventoryComp::GiveEquippable(const TSubclassOf<AEquippableBase>& Class)
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::GiveEquippable()"));
	check(HasAuthority() && GetWorld() && Class)

	AEquippableBase* E = SpawnEquippable(Class);
	AddToSlot(E);
}

AEquippableBase* UInventoryComp::SpawnEquippable(const TSubclassOf<AEquippableBase>& Class) const
{
	// Spawn the thing at the hand socket
	const auto TF = Delegate->GetHandSocketTransform();
	auto Equippable = GetWorld()->SpawnActorDeferred<AEquippableBase>(
		Class,
		TF,
		GetOwner(), // owner actor
		(APawn*)GetOwner(), // instigator pawn // TODO Fix this cast to Pawn
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	UGameplayStatics::FinishSpawningActor(Equippable, TF);

	Equippable->SetActorHiddenInGame(true);

	return Equippable;
}

void UInventoryComp::AddToSlot(AEquippableBase* Equippable)
{
	// Find correct slot
	auto Slot = EInventorySlots::Undefined;

	switch (Equippable->GetInventoryCategory()) {
	case EInventoryCategory::Health: Slot = EInventorySlots::Health; break;
	case EInventoryCategory::Armour: Slot = EInventorySlots::Armour; break;
	case EInventoryCategory::Weapon: Slot = FindGoodWeaponSlot(); break;
	case EInventoryCategory::Throwable: Slot = EInventorySlots::Throwable; break;
	default:
		UE_LOG(LogInventory, Error, TEXT("Unsupported Inventory Category: %d"), Equippable->GetInventoryCategory());
	}


	switch (Slot)
	{
	case EInventorySlots::Primary: break;
	case EInventorySlots::Secondary: break;
	case EInventorySlots::Health: break;
	case EInventorySlots::Armour: break;
	case EInventorySlots::Throwable: ThrowableSlot.Add((AThrowable*)Equippable); break;
	default: 
		UE_LOG(LogInventory, Error, TEXT("Unsupported Inventory Slot: %d"), Slot);
	}
}

bool UInventoryComp::CanGiveThrowable(const TSubclassOf<AThrowable>& ThrowableClass)
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::CanGiveThrowable()"));
	return ThrowableSlot.Num() < ThrowableSlotLimit;
}
void UInventoryComp::GiveThrowableToPlayer(const TSubclassOf<AThrowable>& ThrowableClass)
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::GiveThrowableToPlayer()"));

	GiveEquippable(ThrowableClass);
}


AItemBase* UInventoryComp::GetFirstHealthItemOrNull() const
{
	return HealthSlot.Num() > 0 ? HealthSlot[0] : nullptr;
}
AItemBase* UInventoryComp::GetFirstArmourItemOrNull() const
{
	return ArmourSlot.Num() > 0 ? ArmourSlot[0] : nullptr;
}
AThrowable* UInventoryComp::GetFirstThrowableOrNull() const
{
	return ThrowableSlot.Num() > 0 ? ThrowableSlot[0] : nullptr;
}

void UInventoryComp::GiveWeaponToPlayer(TSubclassOf<class AWeapon> WeaponClass, FWeaponConfig& Config)
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::GiveWeaponToPlayer()"));
	
	check(HasAuthority())

	const auto Weapon = AuthSpawnWeapon(WeaponClass, Config);
	const auto Slot = FindGoodWeaponSlot();
	const auto RemovedWeapon = AssignWeaponToInventorySlot(Weapon, Slot);

	if (RemovedWeapon)
	{
		// If it's the same slot, replay the equip weapon
		RemovedWeapon->ExitInventory();

		// Drop weapon on ground
		TArray<AWeapon*> WeaponArray{ RemovedWeapon };
		SpawnWeaponPickups(WeaponArray);

		RemovedWeapon->Destroy();
		CurrentInventorySlot = EInventorySlots::Undefined;
	}

	EquipSlot(Slot);
}

AWeapon* UInventoryComp::AuthSpawnWeapon(TSubclassOf<AWeapon> weaponClass, FWeaponConfig& Config)
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::AuthSpawnWeapon()"));
	
	check(HasAuthority())

	if (!GetWorld()) return nullptr;

	const auto TF = Delegate->GetHandSocketTransform();// GetMesh()->GetSocketTransform(HandSocketName, RTS_World);


	// Spawn the weapon at the weapon socket
	AWeapon* Weapon = GetWorld()->SpawnActorDeferred<AWeapon>(
		weaponClass,
		TF,
		GetOwner(),
		(APawn*)GetOwner(), // TODO Fix this cast
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!Weapon)
	{
		UE_LOG(LogInventory, Error, TEXT("UInventoryComp::AuthSpawnWeapon - Failed to spawn weapon"));
		return nullptr;
	}

	Weapon->ConfigWeapon(Config);
	Weapon->SetHeroControllerId(Delegate->GetControllerId());

	UGameplayStatics::FinishSpawningActor(Weapon, TF);

	return Weapon;
}

EInventorySlots UInventoryComp::FindGoodWeaponSlot() const
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::FindGoodWeaponSlot()"));

	// Find an empty slot, if one exists
	if (!PrimaryWeaponSlot) return EInventorySlots::Primary;
	if (!SecondaryWeaponSlot) return EInventorySlots::Secondary;

	// If we currently have a weapon selected, replace that.
	if (CurrentInventorySlot == EInventorySlots::Primary || CurrentInventorySlot == EInventorySlots::Secondary)
		return CurrentInventorySlot;

	// If the last inventory slot was a weapon, use that
	if (LastInventorySlot == EInventorySlots::Primary || LastInventorySlot == EInventorySlots::Secondary)
		return LastInventorySlot;

	return EInventorySlots::Primary; // just default to the first slot
}

AWeapon* UInventoryComp::AssignWeaponToInventorySlot(AWeapon* Weapon, EInventorySlots Slot)
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::AssignWeaponToInventorySlot()"));

	bool IsNotAWeaponSlot = Slot != EInventorySlots::Primary && Slot != EInventorySlots::Secondary;
	if (IsNotAWeaponSlot)
	{
		UE_LOG(LogInventory, Error, TEXT("Attempting to equip a weapon into a non weapon slot: %d"), Slot);
		return nullptr;
	}

	const auto ToRemove = GetWeapon(Slot);

	//SetWeapon(Weapon, Slot);
	// This does not free up resources by design! If needed, first get the weapon before overwriting it
	if (Slot == EInventorySlots::Primary) PrimaryWeaponSlot = Weapon;
	if (Slot == EInventorySlots::Secondary) SecondaryWeaponSlot = Weapon;
	Weapon->EnterInventory();

	//// Cleanup previous weapon // TODO Drop this on ground
	//if (Removed) Removed->Destroy();
	return ToRemove;
}

bool UInventoryComp::RemoveEquippableFromInventory(AEquippableBase* Equippable)
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::RemoveEquippableFromInventory()"));
	check(Equippable);

	// TODO Create a collection of items to be cleared out each tick after a second of chillin out. Just to give things a chance to settle

	auto bMustChangeSlot = false;
	bool WasRemoved = false;


	if (Equippable->GetInventoryCategory() == EInventoryCategory::Health)
	{
		const auto Index = HealthSlot.IndexOfByPredicate([Equippable](AItemBase* Item)
			{
				return Item == Equippable;
			});

		if (Index != INDEX_NONE)
		{
			Equippable->ExitInventory();
			HealthSlot.RemoveAt(Index);
			WasRemoved = true;

			bMustChangeSlot = HealthSlot.Num() == 0 && CurrentInventorySlot == EInventorySlots::Health;
		}
	}



	if (Equippable->GetInventoryCategory() == EInventoryCategory::Armour)
	{
		const auto Index = ArmourSlot.IndexOfByPredicate([Equippable](AItemBase* Item)
			{
				return Item == Equippable;
			});

		if (Index != INDEX_NONE)
		{
			Equippable->ExitInventory();
			ArmourSlot.RemoveAt(Index);
			WasRemoved = true;

			bMustChangeSlot = ArmourSlot.Num() == 0 && CurrentInventorySlot == EInventorySlots::Armour;
		}
	}



	if (Equippable->GetInventoryCategory() == EInventoryCategory::Throwable)
	{
		const auto Index = ThrowableSlot.IndexOfByPredicate([Equippable](AThrowable* Item)
			{
				return Item == Equippable;
			});

		if (Index != INDEX_NONE)
		{
			Equippable->ExitInventory();
			ThrowableSlot.RemoveAt(Index);
			WasRemoved = true;

			bMustChangeSlot = ThrowableSlot.Num() == 0 && CurrentInventorySlot == EInventorySlots::Throwable;
		}
	}


	
	if (bMustChangeSlot)
	{
		const auto NewSlot = LastInventorySlot != CurrentInventorySlot ? LastInventorySlot : EInventorySlots::Primary;
		EquipSlot(NewSlot);
	}
	else // Use a new item of the same type in the same slot without an equip time penalty
	{
		// We need to make sure the current item is properly in its equipped state
		
		// TODO Hacky! The item is NOT in any equipped state. A true solution is tricky because:
		// - Can't just make it re-equip as that will replay the equip wait time.
		// - Can't just make it re-equip as that will make auto use items (eg armour) continually auto use until depleted or can't use the item (eg full armour).
		
		// Work around: Make the item in the slot visible even though it's in an unknown equipping state
		// This will break unequip timers when we get to it and is generally inconsistent and bad!
		const auto Held = GetCurrentEquippable();
		Held->SetActorHiddenInGame(false);
	}

	return WasRemoved;
}


// Equipping //////////////////////////////////////////////////////////////////

void UInventoryComp::EquipSlot(const EInventorySlots Slot)
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::EquipSlot()"));

	// Already selected?
	if (CurrentInventorySlot == Slot) return;

	// Desired slot is empty?
	auto NewEquippable = GetEquippable(Slot);
	if (!NewEquippable) return;


	LastInventorySlot = CurrentInventorySlot;
	CurrentInventorySlot = Slot;


	// TODO Some delay on holster


	// Clear any existing Equip timer
	GetWorld()->GetTimerManager().ClearTimer(EquipTimerHandle);


	// Unequip old 
	auto OldEquippable = GetEquippable(LastInventorySlot);
	if (OldEquippable)
	{
		//LogMsgWithRole("Un-equip new slot");
		OldEquippable->Unequip();
		OldEquippable->SetActorHiddenInGame(OldEquippable->ShouldHideWhenUnequipped());
	}


	// Equip new
	if (NewEquippable)
	{
		//LogMsgWithRole("Equip new slot");
		NewEquippable->Equip();
		NewEquippable->SetActorHiddenInGame(true);

		GetWorld()->GetTimerManager().SetTimer(EquipTimerHandle, this, &UInventoryComp::MakeEquippedItemVisible, NewEquippable->GetEquipDuration(), false);
	}

	Delegate->RefreshWeaponAttachments();
}
void UInventoryComp::MakeEquippedItemVisible() const
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::MakeEquippedItemVisible()"));

	//LogMsgWithRole("MakeEquippedItemVisible");
	auto Item = GetEquippable(CurrentInventorySlot);

	if (Item) Item->SetActorHiddenInGame(false);

	Delegate->RefreshWeaponAttachments();
}

void UInventoryComp::NotifyItemIsExpended(AItemBase* Item)
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::NotifyItemIsExpended()"));

	check(HasAuthority())

	const auto WasRemoved = RemoveEquippableFromInventory(Item);
	if (WasRemoved)
	{
		Item->Destroy();
		Delegate->RefreshWeaponAttachments();
	}
}


// Spawning ///////////////////////////////////////////////////////////////////

void UInventoryComp::SpawnHeldWeaponsAsPickups() const
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::SpawnHeldWeaponsAsPickups()"));

	check(GetOwnerRole() == ROLE_Authority);

	// Gather all weapons to drop
	TArray<AWeapon*> WeaponsToDrop{};
	const auto W1 = GetWeapon(EInventorySlots::Primary);
	const auto W2 = GetWeapon(EInventorySlots::Secondary);
	if (W1) WeaponsToDrop.Add(W1);
	if (W2) WeaponsToDrop.Add(W2);

	SpawnWeaponPickups(WeaponsToDrop);
}
void UInventoryComp::SpawnWeaponPickups(TArray<AWeapon*>& Weapons) const
{
	UE_LOG(LogInventory, Verbose, TEXT("UInventoryComp::SpawnWeaponPickups()"));

	// Gather the Pickup Class types to spawn
	TArray<TTuple<TSubclassOf<AWeaponPickupBase>, FWeaponConfig>> PickupClassesToSpawn{};
	int Count = 0;

	for (auto W : Weapons)
	{
		if (W && W->PickupClass && W->HasAmmo())
		{
			// Spawn location algorithm: Alternative between in front and behind player location
			const int FacingFactor = Count % 2 == 0 ? 1 : -1;
			auto Loc = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 30 * FacingFactor;

			auto Params = FActorSpawnParameters{};
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			auto WeaponPickup = GetWorld()->SpawnActor<AWeaponPickupBase>(W->PickupClass, Loc, FRotator{}, Params);
			if (WeaponPickup)
			{
				WeaponPickup->SetWeaponConfig(FWeaponConfig{ W->GetAmmoInClip(), W->GetAmmoInPool() });
				WeaponPickup->bIsSingleUse = true;
				WeaponPickup->SetLifeSpan(60);

				Count++;
			}
		}
	}
}

