// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemArmour.h"
#include "Interfaces/AffectableInterface.h"


bool AItemArmour::CanApplyItem(IAffectableInterface* const Affectable)
{
	UE_LOG(LogTemp, Warning, TEXT("AItemArmour::CanApplyItem"));


	return Affectable ? Affectable->CanGiveArmour() : true; // default to yes

}

void AItemArmour::ApplyItem(IAffectableInterface* const Affectable)
{
	UE_LOG(LogTemp, Warning, TEXT("AItemArmour::ApplyItem"));

	if (Affectable) Affectable->AuthTryGiveArmour(Armour);
}
