// Fill out your copyright notice in the Description page of Project Settings.


#include "ArmourPickup.h"

bool AArmourPickup::TryApplyAffect(IAffectableInterface* const Affectable)
{
	return Affectable->TryGiveArmour(ArmourRestored);
}
