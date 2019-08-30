// Fill out your copyright notice in the Description page of Project Settings.


#include "EquippableBase.h"
#include "InventoryComp.h"
#include "Engine/World.h"
#include "UnrealNetwork.h"


AEquippableBase::AEquippableBase()
{
	SetReplicates(true);
	PrimaryActorTick.bCanEverTick = true;
}


// Replication ////////////////////////////////////////////////////////////////

void AEquippableBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Just the owner
	DOREPLIFETIME_CONDITION(AEquippableBase, EquippedStatus, COND_OwnerOnly);
}




void AEquippableBase::Equip(float DurationOverride)
{
	ClearTimers();

	EquippedStatus = EEquipState::Equipping;
	OnEquipStarted();

	
	const float EquipTime = DurationOverride >= 0 ? DurationOverride : EquipDuration;
	
	if (EquipTime > SMALL_NUMBER)
	{
		GetWorld()->GetTimerManager().SetTimer(
			EquipTimerHandle, this, &AEquippableBase::EquipFinish, EquipTime, false);

		OnEquipping.Broadcast(this);
	}
	else
	{
		OnEquipping.Broadcast(this);
		EquipFinish();
	}
}

void AEquippableBase::EquipFinish()
{
	EquippedStatus = EEquipState::Equipped;
	OnEquipFinished();
	OnEquipped.Broadcast(this);
}

void AEquippableBase::Unequip()
{
	ClearTimers();

	EquippedStatus = EEquipState::UnEquipping;
	OnUnEquipStarted();
	
	if (UnEquipDuration > SMALL_NUMBER)
	{
		GetWorld()->GetTimerManager().SetTimer(
			UnEquipTimerHandle, this, &AEquippableBase::UnEquipFinish, UnEquipDuration, false);

		OnUnEquipping.Broadcast(this);
	}
	else
	{
		OnUnEquipping.Broadcast(this);
		UnEquipFinish();
	}
}

void AEquippableBase::UnEquipFinish()
{
	EquippedStatus = EEquipState::UnEquipped;
	OnUnEquipFinished();
	OnUnEquipped.Broadcast(this);
}

bool AEquippableBase::Is(EInventoryCategory Category)
{
	return GetInventoryCategory() == Category;
}

void AEquippableBase::ClearTimers()
{
	GetWorldTimerManager().ClearTimer(EquipTimerHandle);
	GetWorldTimerManager().ClearTimer(UnEquipTimerHandle);
}
