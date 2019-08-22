// Fill out your copyright notice in the Description page of Project Settings.


#include "EquippableBase.h"
#include "InventoryComp.h"

AEquippableBase::AEquippableBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AEquippableBase::Equip()
{
	OnEquipStarted();
	// TODO Call multicast event
	// TODO Call blueprint implementable method

	OnEquipFinished();
	// TODO Call multicast event
	// TODO Call blueprint implementable method
}

void AEquippableBase::Unequip()
{
	OnUnEquipStarted();
	// TODO Call multicast event
	// TODO Call blueprint implementable method

	OnUnEquipFinished();
	// TODO Call multicast event
	// TODO Call blueprint implementable method
}

void AEquippableBase::BeginPlay()
{
	Super::BeginPlay();
}
void AEquippableBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


bool AEquippableBase::Is(EInventoryCategory Category)
{
	return GetInventoryCategory() == Category;
}