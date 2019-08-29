// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EquippableBase.h"

#include "Throwable.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogThrowable, Display, All);

/**
 * 
 */
UCLASS(Abstract)
class MEATREALM_API AThrowable : public AEquippableBase
{
	GENERATED_BODY()

public: // Data ///////////////////////////////////////////////////////////////
protected: // Data ////////////////////////////////////////////////////////////
private: // Data //////////////////////////////////////////////////////////////
		UPROPERTY(VisibleAnywhere)
		USceneComponent* RootComp = nullptr;

	UPROPERTY(VisibleAnywhere)
		USkeletalMeshComponent* SkeletalMeshComp = nullptr;





public: // Methods ////////////////////////////////////////////////////////////
	AThrowable();
	
protected: // Methods /////////////////////////////////////////////////////////
private: // Methods ///////////////////////////////////////////////////////////

	void BeginPlay() override;

	// Input
	void OnPrimaryPressed() override;
	void OnPrimaryReleased() override;
	void OnSecondaryPressed() override;
	void OnSecondaryReleased() override;
	
	/* AEquippableBase */
	void EnterInventory() override;
	void ExitInventory() override;
	void OnEquipStarted() override;
	void OnEquipFinished() override;
	void OnUnEquipStarted() override;
	void OnUnEquipFinished() override;
	bool ShouldHideWhenUnequipped() override { return true; }
	EInventoryCategory GetInventoryCategory() override { return EInventoryCategory::Throwable; }
	/* End AEquippableBase */

	// Replication
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEquipStarted();
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerEquipFinished();
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerUnEquipStarted();
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerUnEquipFinished();
	
	// Helpers
	void LogMsgWithRole(FString message) const;
	static FString GetEnumText(ENetRole role);
	FString GetRoleText() const;
};
