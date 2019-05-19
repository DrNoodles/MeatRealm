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

	UPROPERTY(EditAnywhere)
		float ShotsPerSecond = 1.0f;

	UPROPERTY(EditAnywhere)
		bool bFullAuto = true;

	UPROPERTY(EditAnywhere)
		bool bUseClip = true;

	UPROPERTY(EditAnywhere)
		int ClipSize = 10;

	UPROPERTY(EditAnywhere)
		float ReloadTime = 3;


	UFUNCTION(Server, Reliable, WithValidation)
		void RPC_Fire_OnServer();

	UFUNCTION(NetMulticast, Reliable)
		void RPC_Fire_RepToClients();

private:
	UFUNCTION()
		void Shoot();

	void ClientFireStart();
	void ClientReloadStart();
	void ClientReloadEnd();
	void ClientFireEnd();

	bool CanReload() const;
	bool NeedsReload() const;

	void LogMsgWithRole(FString message);
	FString GetRoleText();

	int AmmoInClip;
	bool bCanAction;
	bool bTriggerPulled;
	bool bHasActionedThisTriggerPull;
	bool bReloadQueued;
};
