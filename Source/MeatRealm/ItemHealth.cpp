// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemHealth.h"
#include "Interfaces/AffectableInterface.h"


void AItemHealth::ApplyItem(IAffectableInterface* const Affectable)
{
	UE_LOG(LogTemp, Warning, TEXT("Apply Health!"));
	
	if (Affectable) Affectable->AuthTryGiveHealth(Health);
}
