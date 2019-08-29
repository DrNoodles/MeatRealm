// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "EquippableBase.h"

#include "ItemBase.generated.h"

class AHeroCharacter;
class IAffectableInterface;
class UInventoryComp;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUsageDenied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUsageStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUsageSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUsageCancelled);

UCLASS()
class MEATREALM_API AItemBase : public AEquippableBase
{
	GENERATED_BODY()


public:
protected:
	UPROPERTY(BlueprintReadOnly)
		float UsageProgress = 0;

	UPROPERTY(BlueprintReadOnly)
		bool bIsInUse;


	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FUsageDenied OnUsageDenied;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FUsageStarted OnUsageStarted;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FUsageSuccess OnUsageSuccess;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FUsageCancelled OnUsageCancelled;


private:
	FDateTime UsageStartTime;
	FTimerHandle UsageTimerHandle;
	IAffectableInterface* Recipient = nullptr;

	UPROPERTY(EditAnywhere)
		float UsageDuration = 2;

	// When true the player much hold the use key for the duration. When false it's a use to start, another use to cancel
	UPROPERTY(EditAnywhere)
		bool bIsHoldToUse = false;

	UPROPERTY(EditAnywhere)
		bool bIsAutoUseOnEquip = true;

	UPROPERTY(VisibleAnywhere)
		USceneComponent* RootComp = nullptr;

	UPROPERTY(VisibleAnywhere)
		USkeletalMeshComponent* SkeletalMeshComp = nullptr;


public:
	AItemBase();


	void OnPrimaryPressed() override;
	void OnPrimaryReleased() override;
	void OnSecondaryPressed() override;
	void OnSecondaryReleased() override;
	
	bool IsAutoUseOnEquip() const { return bIsAutoUseOnEquip; }
	bool IsInUse() const { return bIsInUse; }
	void SetRecipient(IAffectableInterface* const TheRecipient);

	/* AEquippableBase */
	void OnEquipStarted() override;
	void OnEquipFinished() override;
	void OnUnEquipStarted() override;
	void OnUnEquipFinished() override;
	void EnterInventory() override;
	void ExitInventory() override;
	virtual bool ShouldHideWhenUnequipped() override { return true; }
	virtual EInventoryCategory GetInventoryCategory() override
	{
		unimplemented();
		return EInventoryCategory::Undefined;
	}
	/* End AEquippableBase */

	

protected:
	virtual bool CanApplyItem(IAffectableInterface* Affectable)
	{
		unimplemented();
		return false;
	}
	virtual void ApplyItem(IAffectableInterface* Affectable)
	{
		unimplemented();
	}


	UFUNCTION(BlueprintCallable)
		float GetUsageTimeRemaining() const;


private:
	virtual void BeginPlay() override;

	void Tick(float DeltaSeconds) override;


	void UsePressed();
	void UseReleased();
	void Cancel();
	void UseComplete();
	void StopAnyUsage();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerUsePressed();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerUseReleased();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerCancel();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerEquip();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerUnequip();

	// Expects object that implements IAffectableInterface - Because Interfaces cant be replicated
	UFUNCTION(Client, Reliable)
		void ClientSetRecipient(UObject* Affectable);
};
