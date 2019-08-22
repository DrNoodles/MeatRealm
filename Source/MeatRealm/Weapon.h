// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponReceiverComponent.h"
#include "EquippableBase.h"

#include "Weapon.generated.h"

class AHeroCharacter;
class UWeaponReceiverComponent;
class IAffectableInterface;
class UArrowComponent;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UPointLightComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReloadStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReloadEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShotFired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAmmoWarning);

USTRUCT()
struct FWeaponConfig
{
	GENERATED_BODY()

	int AmmoInClip = -1;
	int AmmoInPool = -1;
};

UCLASS()
class MEATREALM_API AWeapon : public AEquippableBase, public IReceiverComponentDelegate
{
	GENERATED_BODY()

public:

	// Associated pickup class. Used to drop this weapon.
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
		TSubclassOf<class AWeaponPickupBase> PickupClass;


protected:

	/// Components

	UPROPERTY(VisibleAnywhere)
		USceneComponent* RootComp = nullptr;

	UPROPERTY(VisibleAnywhere)
		USkeletalMeshComponent* SkeletalMeshComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UArrowComponent* MuzzleLocationComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UPointLightComponent* MuzzleLightComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UWeaponReceiverComponent* ReceiverComp;

	// Projectile class to spawn.
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
		TSubclassOf<class AProjectile> ProjectileClass;


	//// Configure the gun

	// Time (seconds) to holster the weapon
	//UPROPERTY(EditAnywhere)
	//	float HolsterDuration = 0.5;

	UPROPERTY(EditAnywhere)
		float DrawDuration = 1;

	UPROPERTY(EditAnywhere)
		float AdsMovementScale = 0.70;


	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FShotFired OnShotFired;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FAmmoWarning OnAmmoWarning;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FReloadStarted OnReloadStarted;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FReloadEnded OnReloadEnded;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FString WeaponName = "NoNameWeapon";

private:
	
	uint32 HeroControllerId;



public:
	AWeapon();

	/* AEquippableBase */
	void OnEquipStarted() override;
	void OnEquipFinished() override;
	void OnUnEquipStarted() override;
	void OnUnEquipFinished() override;
	float GetEquipDuration() override { return DrawDuration; }
	void EnterInventory() override;
	void ExitInventory() override;
	EInventoryCategory GetInventoryCategory() override { return EInventoryCategory::Weapon; }
	virtual bool ShouldHideWhenUnequipped() override { return false; }
	/* End AEquippableBase */

	void ConfigWeapon(FWeaponConfig& Config) const;

	void Input_PullTrigger();
	void Input_ReleaseTrigger();
	void Input_Reload();
	void Input_AdsPressed();
	void Input_AdsReleased();
	bool CanGiveAmmo();
	bool TryGiveAmmo();
	void SetHeroControllerId(uint32 HeroControllerUid) { this->HeroControllerId = HeroControllerUid; }
	float GetAdsMovementScale() const { return AdsMovementScale; }
	//float GetHolsterDuration() const { return HolsterDuration; }

	/* IReceiverComponentDelegate */
	void ShotFired() override;
	void AmmoInClipChanged(int AmmoInClip) override;
	void AmmoInPoolChanged(int AmmoInPool) override;
	void InReloadingChanged(bool IsReloading) override;
	void OnReloadProgressChanged(float ReloadProgress) override;
	bool SpawnAProjectile(const FVector& Direction) override;
	FVector GetBarrelDirection() override;
	FVector GetBarrelLocation() override;
	const UArrowComponent* GetMuzzleComponent() const { return MuzzleLocationComp; }
	float GetDrawDuration() override;

	AActor* GetOwningPawn() override;
	FString GetWeaponName() override;
	bool IsEquipping() const { return ReceiverComp->IsEquipping(); }
	bool IsReloading() const { return ReceiverComp->IsReloading(); }
	void CancelAnyReload();

	int GetAmmoInClip() const { return ReceiverComp->GetState().AmmoInClip; }
	int GetAmmoInPool() const { return ReceiverComp->GetState().AmmoInPool; }
	bool HasAmmo() const { return ReceiverComp->GetState().AmmoInClip + ReceiverComp->GetState().AmmoInPool > 0; }

	/* End IReceiverComponentDelegate */


	// How much the weapon should aim up or down in degrees. Eg 90 is up, 0 straight ahead, and -90 is down.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float PitchAimOffset = 0;

protected:
private:

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_Equip();
	
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_Unequip();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_PullTrigger();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_ReleaseTrigger();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_Reload();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_AdsPressed();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_AdsReleased();

	UFUNCTION(NetMulticast, Reliable)
		void MultiRPC_NotifyOnShotFired();

	UFUNCTION(Client, Reliable)
		void ClientRPC_NotifyOnAmmoWarning();


	void LogMsgWithRole(FString message) const;
	FString GetRoleText() const;
};

