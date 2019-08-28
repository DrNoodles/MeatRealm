// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemPickupBase.h"

bool AItemPickupBase::CanInteract(IAffectableInterface* const Affectable, float& OutDelay)
{
	bool bCanInteract = Super::CanInteract(Affectable, OutDelay);
	if (bCanInteract)
	{
		bCanInteract = Affectable->CanGiveItem(ItemClass, OUT OutDelay);
	}
	return bCanInteract;
}

bool AItemPickupBase::TryApplyAffect(IAffectableInterface* const Affectable)
{
	if (ItemClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Set the item pickup class to spawn in a derived Blueprint"));
		return false;
	}

	return Affectable->TryGiveItem(ItemClass);
}
