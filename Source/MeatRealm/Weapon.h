// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponReceiverComponent.h"
#include "Interfaces/Equippable.h"

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

UCLASS()
class MEATREALM_API AWeapon : public AActor, public IReceiverComponentDelegate, public IEquippable
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
	UPROPERTY(EditAnywhere)
		bool IsBuff = false;

	UPROPERTY(EditAnywhere)
		float DrawDurationBuffMultiplier	= 0.5;

	UPROPERTY(EditAnywhere)
		float AdsMovementScaleBuffFactor = 0.5;
	
	uint32 HeroControllerId;



public:
	AWeapon();

	/// [Server, Local]
	/* IEquippable */
	void Equip() override;
	void Unequip() override;
	float GetEquipDuration() override { return IsBuff ? DrawDuration * DrawDurationBuffMultiplier : DrawDuration; }
	void SetHidden(bool bIsHidden) override { SetActorHiddenInGame(bIsHidden); }
	void EnterInventory() override;
	void ExitInventory() override;
	EInventoryCategory GetInventoryCategory() override { return EInventoryCategory::Weapon; }
	virtual bool ShouldHideWhenUnequipped() override { return false; }
	void SetDelegate(AHeroCharacter* Delegate) { }
	/* End IEquippable */

	void Input_PullTrigger();
	void Input_ReleaseTrigger();
	void Input_Reload();
	void Input_AdsPressed();
	void Input_AdsReleased();
	bool CanGiveAmmo();
	bool TryGiveAmmo();
	void SetHeroControllerId(uint32 HeroControllerUid) { this->HeroControllerId = HeroControllerUid; }
	float GetAdsMovementScale() const { return IsBuff
		? AdsMovementScale + (1 - AdsMovementScale) * AdsMovementScaleBuffFactor
		: AdsMovementScale; }
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

	bool IsWeaponBuff() const { return IsBuff; }

	/* End IReceiverComponentDelegate */


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

	void BeginPlay() override;

	void LogMsgWithRole(FString message);
	FString GetRoleText();


};

