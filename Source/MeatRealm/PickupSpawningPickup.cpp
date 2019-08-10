// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupSpawningPickup.h"
#include "Engine/World.h"

bool APickupSpawningPickup::TryApplyAffect(IAffectableInterface* const Affectable)
{
	if (PickupClasses.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Set the pickup class(es) to spawn in a derived Blueprint"));
		return false;
	}


	// Spawn a random pickup into the world
	
	const auto Choice = FMath::RandRange(0, PickupClasses.Num() - 1);

	FActorSpawnParameters Params{};
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	auto* Pickup = GetWorld()->SpawnActorAbsolute<APickupBase>(PickupClasses[Choice], GetActorTransform(), Params);
	if (Pickup)
	{
		Pickup->bIsSingleUse = true;
	}

	return Pickup != nullptr;
}
