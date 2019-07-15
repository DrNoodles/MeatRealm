// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Interfaces/Equippable.h"

#include "ItemBase.generated.h"

class AHeroCharacter;
class IAffectableInterface;

//DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUsageSuccess);

UCLASS()
class MEATREALM_API AItemBase : public AActor, public IEquippable
{
	GENERATED_BODY()


public:
protected:
	UPROPERTY(BlueprintReadOnly)
		float UsageProgress = 0;

	UPROPERTY(BlueprintReadOnly)
		bool bIsInUse;

	UPROPERTY(EditAnywhere)
		float UsageDuration = 2;

	UPROPERTY(EditAnywhere)
		float EquipDuration = 0.5;

	//UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
	//	FUsageSuccess OnUsageSuccess;

private:
	FDateTime UsageStartTime;
	FTimerHandle UsageTimerHandle;
	IAffectableInterface* Recipient = nullptr;
	AHeroCharacter* Delegate = nullptr; // TODO This is a horrible coupling. Make it a UInventoryComponent. Using an interface is a fools errand in UE4


	UPROPERTY(VisibleAnywhere)
		USceneComponent* RootComp = nullptr;

	UPROPERTY(VisibleAnywhere)
		USkeletalMeshComponent* SkeletalMeshComp = nullptr;


public:
	AItemBase();

	void UseStart();
	void UseStop();

	bool IsInUse() const { return bIsInUse; }
	void SetRecipient(IAffectableInterface* const TheRecipient);

	/* IEquippable */
	void Equip() override;
	void Unequip() override;
	float GetEquipDuration() override { return EquipDuration; }
	void SetHidden(bool bIsHidden) override { SetActorHiddenInGame(bIsHidden); }
	void EnterInventory() override;
	void ExitInventory() override;
	void SetDelegate(AHeroCharacter* Delegate) override;
	virtual bool ShouldHideWhenUnequipped() override { return true; }
	virtual EInventoryCategory GetInventoryCategory() override
	{
		unimplemented();
		return EInventoryCategory::Undefined;
	}
	/* End IEquippable */

protected:
	virtual bool CanApplyItem(IAffectableInterface* Affectable)
	{
		unimplemented();
		return false;
	}
	//virtual void ApplyItem(IAffectableInterface* const Affectable) PURE_VIRTUAL(AItemBase::ApplyItem, );
	virtual void ApplyItem(IAffectableInterface* Affectable)
	{
		unimplemented();
	}
	

private:
	virtual void BeginPlay() override;
	void Tick(float DeltaSeconds) override;

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerUseStart();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerUseStop();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerEquip();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerUnequip();

	// Expects object that implements IAffectableInterface - Because Interfaces cant be replicated
	UFUNCTION(Client, Reliable)
		void ClientSetRecipient(UObject* Affectable);
};
