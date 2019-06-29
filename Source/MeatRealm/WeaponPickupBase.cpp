// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponPickupBase.h"

bool AWeaponPickupBase::CanInteract(IAffectableInterface* const Affectable, float& OutDelay)
{
	bool bCanInteract = Super::CanInteract(Affectable, OutDelay);
	if (bCanInteract)
	{
		bCanInteract = Affectable->CanGiveWeapon(WeaponClass, OUT OutDelay);
	}
	return bCanInteract;
}

bool AWeaponPickupBase::TryApplyAffect(IAffectableInterface* const Affectable)
{
	if (WeaponClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Set the weapon class to spawn in a derived Blueprint"));
		return false;
	}

	return Affectable->AuthTryGiveWeapon(WeaponClass);
}
