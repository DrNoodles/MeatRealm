// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

#include "ItemBase.generated.h"

class IAffectableInterface;

//DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUsageStarted);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUsageCancelled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUsageSuccess);

UCLASS()
class MEATREALM_API AItemBase : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	float UsageDuration = 2;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FUsageSuccess OnUsageSuccess;
	
private:
	FTimerHandle UsageTimerHandle;

public:
	AItemBase();

	void UseStart(IAffectableInterface* const Affectable);
	void UseStop();
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
