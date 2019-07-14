// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemBase.h"

#include "ItemArmour.generated.h"

UCLASS()
class MEATREALM_API AItemArmour : public AItemBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
		float Armour = 25;

private:
	bool CanApplyItem(IAffectableInterface* const Affectable) override;
	void ApplyItem(IAffectableInterface* const Affectable) override;
	EInventoryCategory GetInventoryCategory() override { return EInventoryCategory::Armour; }
};
