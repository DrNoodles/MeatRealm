// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Public/TimerManager.h"

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


private:
	FTimerHandle CycleTimerHandle;


	/// Components

	UPROPERTY(VisibleAnywhere)
		USceneComponent* Root = nullptr;

	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* Mesh = nullptr;

	UPROPERTY(VisibleAnywhere)
		UArrowComponent* ShotSpawnLocation = nullptr;

	UPROPERTY(EditAnywhere)
		float ShotsPerSecond = 1.0f;

	UPROPERTY(EditAnywhere)
		float ShotDamage = 1.0f;

	UPROPERTY(EditAnywhere)
		bool bRepeats = true;
	


	UFUNCTION()
	void OnFire();

};
