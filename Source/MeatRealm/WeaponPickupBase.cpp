// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponPickupBase.h"

bool AWeaponPickupBase::CanInteract(IAffectableInterface* const Affectable, float& OutDelay)
{
	if (WeaponClasses.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Set the weapon class to spawn in a derived Blueprint"));
		return false;
	}


	bool bCanInteract = Super::CanInteract(Affectable, OutDelay);
	if (bCanInteract)
	{
		bCanInteract = Affectable->CanGiveWeapon(WeaponClasses[0], OUT OutDelay); // TODO Solve this random issue, might have to select a seed on spawn so we can query CanGiveWeapon with the same class that it'll inevitably spawn
	}
	return bCanInteract;
}

bool AWeaponPickupBase::TryApplyAffect(IAffectableInterface* const Affectable)
{
	if (WeaponClasses.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Set the weapon class to spawn in a derived Blueprint"));
		return false;
	}

	const auto Choice = FMath::RandRange(0, WeaponClasses.Num() - 1);
	return Affectable->AuthTryGiveWeapon(WeaponClasses[Choice]);
}
