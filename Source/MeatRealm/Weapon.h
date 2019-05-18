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
	void PullTrigger();
	void ReleaseTrigger();


	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Gameplay)
	//	UArrowComponent* ShotSpawnLocation = nullptr;

public:
	FTimerHandle CycleTimerHandle;


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
		bool bRepeats = true;



	//UFUNCTION()
	//	void OnInputFire();

	UFUNCTION(Server, Reliable, WithValidation)
		void RPC_Fire_OnServer();

	UFUNCTION(NetMulticast, Reliable)
		void RPC_Fire_RepToClients();

	UFUNCTION()
	void Shoot();


private:
	void LogMethodWithRole(FString message);
	FString GetRoleText();

};
