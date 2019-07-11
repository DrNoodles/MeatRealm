// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemBase.generated.h"

class IAffectableInterface;

UCLASS(Abstract)
class MEATREALM_API AItemBase : public AActor
{
	GENERATED_BODY()

public:
	float UsageDuration = 2;

public:	
	AItemBase();

	void StartUse(IAffectableInterface* const Affectable);
	void StopUse();
	void Equip();
	void Unequip();

protected:
	//virtual void ApplyItem(IAffectableInterface* const Affectable) PURE_VIRTUAL(AItemBase::ApplyItem, );
	virtual void ApplyItem(IAffectableInterface* const Affectable)
	{
		unimplemented();
	}


private:
	//virtual void BeginPlay() override;
	//virtual void Tick(float DeltaTime) override;
};
