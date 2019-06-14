// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponPickupBase.h"

bool AWeaponPickupBase::TryApplyAffect(IAffectableInterface* const Affectable)
{
	if (WeaponClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Set the weapon class to spawn in a derived Blueprint"));
		return false;
	}

	return Affectable->AuthTryGiveWeapon(WeaponClass);
}
