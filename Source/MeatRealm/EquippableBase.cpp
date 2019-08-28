// Fill out your copyright notice in the Description page of Project Settings.


#include "EquippableBase.h"
#include "InventoryComp.h"
#include "Engine/World.h"
#include "UnrealNetwork.h"


AEquippableBase::AEquippableBase()
{
	PrimaryActorTick.bCanEverTick = true;
}


// Replication ////////////////////////////////////////////////////////////////

void AEquippableBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Just the owner
	DOREPLIFETIME_CONDITION(AEquippableBase, EquippedStatus, COND_OwnerOnly);
}




void AEquippableBase::Equip()
{
	ClearTimers();

	EquippedStatus = EEquipState::Equipping;
	OnEquipStarted();
	// TODO Call multicast event
	// TODO Call blueprint implementable method

	GetWorld()->GetTimerManager().SetTimer(EquipTimerHandle, this, &AEquippableBase::EquipFinish, EquipDuration, false);
}
void AEquippableBase::EquipFinish()
{
	EquippedStatus = EEquipState::Equipped;
	OnEquipFinished();
	// TODO Also Call multicast event
	// TODO Also Call blueprint implementable method
}

void AEquippableBase::Unequip()
{
	ClearTimers();

	EquippedStatus = EEquipState::UnEquipping;
	OnUnEquipStarted();
	// TODO Call multicast event
	// TODO Call blueprint implementable method


	GetWorld()->GetTimerManager().SetTimer(UnEquipTimerHandle, this, &AEquippableBase::UnEquipFinish, UnEquipDuration, false);
	
}
void AEquippableBase::UnEquipFinish()
{
	EquippedStatus = EEquipState::UnEquipped;
	OnUnEquipFinished();
	// TODO Call multicast event
	// TODO Call blueprint implementable method
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
