// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PickupBase.h"
#include "ArmourPickup.generated.h"

/**
 * 
 */
UCLASS()
class MEATREALM_API AArmourPickup : public APickupBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float ArmourRestored = 25;

protected:
	bool TryApplyAffect(IAffectableInterface* const Affectable) override;

};
