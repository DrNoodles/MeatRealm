// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "PickupBase.h"

#include "PickupSpawnLocation.generated.h"

/**
 * 
 */
UCLASS()
class MEATREALM_API APickupSpawnLocation : public ATargetPoint
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = Pickup)
		TSubclassOf<class APickupBase> PickupClass;

	UPROPERTY(EditDefaultsOnly, Category = Pickup)
		TSubclassOf<AActor> PreviewClass;

};