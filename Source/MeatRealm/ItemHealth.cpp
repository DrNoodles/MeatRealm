// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemHealth.h"
#include "Interfaces/AffectableInterface.h"


bool AItemHealth::CanApplyItem(IAffectableInterface* const Affectable)
{
	UE_LOG(LogTemp, Warning, TEXT("AItemHealth::CanApplyItem"));
	
	
	return Affectable ? Affectable->CanGiveHealth() : true; // default to yes

}

void AItemHealth::ApplyItem(IAffectableInterface* const Affectable)
{
	UE_LOG(LogTemp, Warning, TEXT("AItemHealth::ApplyItem"));

	if (Affectable) Affectable->AuthTryGiveHealth(Health);
}
