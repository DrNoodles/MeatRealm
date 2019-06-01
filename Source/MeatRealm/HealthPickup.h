// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PickupBase.h"
#include "HealthPickup.generated.h"

/**
 * 
 */
UCLASS()
class MEATREALM_API AHealthPickup : public APickupBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int HealthRestored = 10;

protected:
	bool TryApplyAffect(IAffectableInterface* const Affectable) override;
};
