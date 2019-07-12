// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Interfaces/Equippable.h"

#include "ItemBase.generated.h"

class IAffectableInterface;

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
	IAffectableInterface* Recipient = nullptr;

	UPROPERTY(VisibleAnywhere)
		USceneComponent* RootComp = nullptr;

	UPROPERTY(VisibleAnywhere)
		USkeletalMeshComponent* SkeletalMeshComp = nullptr;


public:
	AItemBase();

	void UseStart();
	void UseStop();
	void SetRecipient(IAffectableInterface* const TheRecipient);

	/* IEquippable */
	void Equip() override;
	void Unequip() override;
	float GetEquipDuration() override { return EquipDuration; }
	void SetHidden(bool bIsHidden) override { SetActorHiddenInGame(bIsHidden); }
	void EnterInventory() override;
	void ExitInventory() override;
	virtual EInventoryCategory GetInventoryCategory() override
	{
		unimplemented();
		return EInventoryCategory::Undefined;
	}
	/* End IEquippable */

protected:
	//virtual void ApplyItem(IAffectableInterface* const Affectable) PURE_VIRTUAL(AItemBase::ApplyItem, );
	virtual void ApplyItem(IAffectableInterface* Affectable)
	{
		unimplemented();
	}


private:
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerUseStart();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerUseStop();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerEquip();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerUnequip();
};
