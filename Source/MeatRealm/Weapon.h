// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Weapon.generated.h"

class UArrowComponent;
class USceneComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShotFired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAmmoWarning);

enum ReloadStates
{
	Nothing = 0,
	Starting = 1,
	InProgress = 2,
	Finishing = 3,
};

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
	uint32 HeroControllerId;
	void SetHeroControllerId(uint32 HeroControllerUid) { this->HeroControllerId = HeroControllerUid; }

public:
	FTimerHandle CanActionTimerHandle;


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

	UPROPERTY(EditAnywhere)
		float ShotsPerSecond = 1.0f;

	UPROPERTY(EditAnywhere)
		bool bFullAuto = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float AdsSpread = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float HipfireSpread = 20;

	UPROPERTY(EditAnywhere)
		int AmmoPoolSize = 50;

	UPROPERTY(EditAnywhere)
		int AmmoPoolGiven = 40;

	UPROPERTY(EditAnywhere)
		int AmmoGivenPerPickup = 10;

	UPROPERTY(EditAnywhere)
		bool bUseClip = true;

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
	UPROPERTY(BlueprintReadOnly)
		bool bIsReloading = false;

	// Used for UI binding
	UPROPERTY(BlueprintReadOnly)
	float ReloadProgress = 0.f;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FShotFired OnShotFired;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FAmmoWarning OnAmmoWarning;

private:
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
	void AuthReloadStart();
	void AuthReloadEnd();
	void AuthFireEnd();

	bool CanReload() const;
	bool NeedsReload() const;
	bool IsMatchInProgress();

	void LogMsgWithRole(FString message);
	FString GetRoleText();

	bool bCanAction;
	bool bTriggerPulled;
	bool bAdsPressed;
	bool bIsInAdsMode = false;
	bool bHasActionedThisTriggerPull;
	bool bReloadQueued;


	// 0 nothing, 1 reload starting, 2 reloading, 3 reload finishing
	UPROPERTY(Replicated)
		int ReloadState = 0;

	FDateTime ReloadStartTime;
};

