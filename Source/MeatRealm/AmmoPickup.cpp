// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"

bool AAmmoPickup::TryApplyAffect(IAffectableInterface* const Affectable)
{
	return Affectable->TryGiveAmmo();
}
