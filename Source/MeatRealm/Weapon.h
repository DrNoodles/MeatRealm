// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Public/TimerManager.h"
#include "Projectile.h"

#include "Weapon.generated.h"

class UArrowComponent;
class USceneComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShotFired);

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

	void Input_PullTrigger();
	void Input_ReleaseTrigger();
	void Input_Reload();
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
		float HipfireSpread = 20;

	UPROPERTY(EditAnywhere)
		int AmmoPoolSize = 50;

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

	UPROPERTY(BlueprintReadOnly, Replicated)
	bool bIsReloading;

	UPROPERTY(BlueprintReadOnly)
	float ReloadProgress = 0.f;




	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FShotFired OnShotFired;

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_Reload();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_PullTrigger();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_ReleaseTrigger();

	UFUNCTION(Server, Reliable, WithValidation)
		void RPC_Fire_OnServer();

	UFUNCTION(NetMulticast, Reliable)
		void MultiRPC_Fired();


private:
	UFUNCTION()
		void Shoot();

	TArray<FVector> CalcShotPattern() const;
	bool SpawnAProjectile(const FVector& Direction) const;

	void ClientFireStart();
	void ClientReloadStart();
	void ClientReloadEnd();
	void ClientFireEnd();

	bool CanReload() const;
	bool NeedsReload() const;

	void LogMsgWithRole(FString message);
	FString GetRoleText();

	bool bCanAction;
	bool bTriggerPulled;
	bool bHasActionedThisTriggerPull;
	bool bReloadQueued;

	UPROPERTY(Replicated)
	FDateTime ReloadStartTime;
};
