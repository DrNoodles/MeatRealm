// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"

bool AAmmoPickup::CanInteract(IAffectableInterface* const Affectable, float& OutDelay)
{
	bool bCanInteract = Super::CanInteract(Affectable, OutDelay);
	if (bCanInteract)
	{
		bCanInteract = Affectable->CanGiveAmmo();
	}
	return bCanInteract;
}

bool AAmmoPickup::TryApplyAffect(IAffectableInterface* const Affectable)
{
	return Affectable->AuthTryGiveAmmo();
}
