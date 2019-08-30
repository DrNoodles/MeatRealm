// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EquippableBase.h"

#include "Throwable.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogThrowable, Display, All);

class AProjectile;

/**
 * 
 */
UCLASS(Abstract)
class MEATREALM_API AThrowable : public AEquippableBase
{
	GENERATED_BODY()

public: // Data ///////////////////////////////////////////////////////////////
	
protected: // Data ////////////////////////////////////////////////////////////
	UPROPERTY(EditAnywhere)
		float AdsMovementScale = 0.50;
	
	// How much the weapon should aim up or down in degrees. Eg 90 is up, 0 straight ahead, and -90 is down.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float PitchAimOffset = 0;
	
	// Projectile class to spawn.
	UPROPERTY(EditDefaultsOnly)
		TSubclassOf<AProjectile> ProjectileClass;
	
private: // Data //////////////////////////////////////////////////////////////
	UPROPERTY(VisibleAnywhere)
		USceneComponent* RootComp = nullptr;

	UPROPERTY(VisibleAnywhere)
		USkeletalMeshComponent* SkeletalMeshComp = nullptr;

	UPROPERTY(Replicated)
	bool bIsAiming;


	
public: // Methods ////////////////////////////////////////////////////////////
	AThrowable();
	bool IsTargeting() const { return bIsAiming; }
	float GetAdsMovementScale() const { return AdsMovementScale; }

protected: // Methods /////////////////////////////////////////////////////////
	
private: // Methods ///////////////////////////////////////////////////////////

	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;


	// Throw Projectile
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestThrow();
	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MultiDoThrow();
	// [Server]
	void SpawnProjectile();
	// [All Clients]
	void ProjectileThrown();
	
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

	// Aiming
	void VisualiseProjectile() const;
	FVector GetAimLocation() const;
	FRotator GetAimRotator() const;

	void SetAiming(bool NewAiming);
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetAiming(bool NewAiming);

	// Helpers
	void LogMsgWithRole(FString message) const;
	static FString GetEnumText(ENetRole role);
	FString GetRoleText() const;
};
