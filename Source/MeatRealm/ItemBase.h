// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Interfaces/Equippable.h"

#include "ItemBase.generated.h"

class IAffectableInterface;

//DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUsageStarted);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUsageCancelled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUsageSuccess);

UCLASS()
class MEATREALM_API AItemBase : public AActor, public IEquippable
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
		float UsageDuration = 2;

	UPROPERTY(EditAnywhere)
		float EquipDuration = 0.5;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FUsageSuccess OnUsageSuccess;
	
private:
	FTimerHandle UsageTimerHandle;

public:
	AItemBase();

	void UseStart(IAffectableInterface* const Affectable);
	void UseStop();

	/* IEquippable */
	void Equip() override;
	void Unequip() override;
	float GetEquipDuration() override { return EquipDuration; }
	void SetHidden(bool bIsHidden) override { SetActorHiddenInGame(bIsHidden); }
	virtual EInventoryCategory GetInventoryCategory() override
	{
		unimplemented();
		return EInventoryCategory::Undefined;
	}

	/* End IEquippable */


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
