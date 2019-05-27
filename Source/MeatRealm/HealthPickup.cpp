// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"


bool AHealthPickup::TryApplyAffect(IAffectableInterface* const Affectable)
{
	//LogMsgWithRole("TryApplyAffect()");
	return Affectable->TryGiveHealth(HealthRestored);
}