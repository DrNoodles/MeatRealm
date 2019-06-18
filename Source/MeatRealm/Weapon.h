// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Weapon.generated.h"

class UArrowComponent;
class USceneComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReloadStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReloadEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShotFired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAmmoWarning);

UCLASS()
class MEATREALM_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	void RemoteTick(float DeltaTime);
	void AuthTick(float DeltaTime);

	void Input_PullTrigger();
	void Input_ReleaseTrigger();
	void Input_Reload();
	void Input_AdsPressed();
	void Input_AdsReleased();
	bool TryGiveAmmo();

	UFUNCTION(Client, Reliable)
	void ClientRPC_RefreshCurrentInput();
	void Draw();
	void QueueHolster();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString WeaponName = "NoNameWeapon";

	uint32 HeroControllerId;
	void SetHeroControllerId(uint32 HeroControllerUid) { this->HeroControllerId = HeroControllerUid; }





public:
	


	/// Components

	UPROPERTY(VisibleAnywhere)
		USceneComponent* RootComp = nullptr;

	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* MeshComp = nullptr;

	UPROPERTY(VisibleAnywhere)
		UArrowComponent* MuzzleLocationComp = nullptr;



	// Projectile class to spawn.
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
		TSubclassOf<class AProjectile> ProjectileClass;


	// Configure the gun

	// Time (seconds) to holster the weapon
	UPROPERTY(EditAnywhere)
		float HolsterDuration = 1;

	UPROPERTY(EditAnywhere)
		float ShotsPerSecond = 1.0f;

	UPROPERTY(EditAnywhere)
		bool bFullAuto = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float AdsSpread = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float HipfireSpread = 20;

	UPROPERTY(EditAnywhere)
		int AmmoPoolSize = 30;

	UPROPERTY(EditAnywhere)
		int AmmoPoolGiven = 20;

	UPROPERTY(EditAnywhere)
		int AmmoGivenPerPickup = 10;

	UPROPERTY(EditAnywhere)
		bool bUseClip = true;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bUseClip"))
		int ClipSizeGiven = 0;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bUseClip"))
		int ClipSize = 10;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bUseClip"))
		float ReloadTime = 3;

	// The number of projectiles fired per shot
	UPROPERTY(EditAnywhere)
		int ProjectilesPerShot = 1;

	// When ProjectilesPerShot > 1 this ensures all projectiles are spread evenly across the HipfireSpread angle.
	UPROPERTY(EditAnywhere)
		bool bEvenSpread = true;

	// This makes even spreading feel more natural by randomly clumping the shots within the even spread.
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bEvenSpread"))
		bool bSpreadClumping = true;


	// Gun status

	UPROPERTY(BlueprintReadOnly, Replicated)
		int AmmoInClip;

	UPROPERTY(BlueprintReadOnly, Replicated)
		int AmmoInPool;

	// Used for UI binding
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_IsReloadingChanged)
		bool bIsReloading = false;
	UFUNCTION()
		void OnRep_IsReloadingChanged();

	// Used for UI binding
	UPROPERTY(BlueprintReadOnly)
	float ReloadProgress = 0.f;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FShotFired OnShotFired;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FAmmoWarning OnAmmoWarning;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FReloadStarted OnReloadStarted;
	
	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FReloadEnded OnReloadEnded;


private:
	UPROPERTY(EditAnywhere)
		float AdsLineLength = 1500; // cm

	UPROPERTY(EditAnywhere)
		FColor AdsLineColor = FColor{ 255,0,0 };

	UPROPERTY(EditAnywhere)
		float EnemyAdsLineLength = 175; // cm

	UPROPERTY(EditAnywhere)
		FColor EnemyAdsLineColor = FColor{ 255,170,75 };

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_Reload();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_PullTrigger();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_ReleaseTrigger();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_AdsPressed();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_AdsReleased();

	UFUNCTION(NetMulticast, Reliable)
		void MultiRPC_NotifyOnShotFired();

	UFUNCTION(Client, Reliable)
		void ClientRPC_NotifyOnAmmoWarning();


	void SpawnProjectiles() const;

	TArray<FVector> CalcShotPattern() const;
	bool SpawnAProjectile(const FVector& Direction) const;

	void AuthFireStart();
	void AuthFireEnd();
	void AuthHolsterStart();
	void AuthReloadStart();
	void AuthReloadEnd();

	bool CanReload() const;
	bool NeedsReload() const;
	bool IsMatchInProgress();

	void LogMsgWithRole(FString message);
	FString GetRoleText();
	void DrawAdsLine(const FColor& Color, float LineLength) const;

	bool bCanAction;
	bool bTriggerPulled;
	
	UPROPERTY(Replicated)
	bool bAdsPressed;
	
	bool bHasActionedThisTriggerPull;
	bool bReloadQueued;
	bool bHolsterQueued;

	bool bWasReloadingOnHolster;
	FDateTime ClientReloadStartTime;
	FTimerHandle CanActionTimerHandle;

};

