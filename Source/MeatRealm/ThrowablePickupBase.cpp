// Fill out your copyright notice in the Description page of Project Settings.


#include "ThrowablePickupBase.h"

bool AThrowablePickupBase::CanInteract(IAffectableInterface* const Affectable, float& OutDelay)
{
	bool bCanInteract = Super::CanInteract(Affectable, OutDelay);
	if (bCanInteract)
	{
		bCanInteract = Affectable->CanGiveThrowable(PickupClass, OUT OutDelay);
	}
	return bCanInteract;
}

bool AThrowablePickupBase::TryApplyAffect(IAffectableInterface* const Affectable)
{
	if (PickupClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Set the throwable pickup class to spawn in a derived Blueprint"));
		return false;
	}

	return Affectable->TryGiveThrowable(PickupClass);
}
