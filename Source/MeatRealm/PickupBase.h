// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/AffectableInterface.h" // Used in default impl of TryApplyAffect

#include "PickupBase.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UCapsuleComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPickupSpawned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPickupTaken);

UCLASS()
class MEATREALM_API APickupBase : public AActor
{
	GENERATED_BODY()


	/// Data

public:
	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FPickupSpawned OnSpawn;
	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FPickupSpawned OnTaken;
	// In Seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float RespawnDelay = 20;

protected:
	UPROPERTY(EditAnywhere)
		bool bExplicitInteraction = false;

private:
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* MeshComp = nullptr;
	UPROPERTY(VisibleAnywhere)
		USkeletalMeshComponent* SkeletalMeshComp = nullptr;
	UPROPERTY(VisibleAnywhere)
		UCapsuleComponent* CollisionComp = nullptr;
	UPROPERTY(ReplicatedUsing = OnRep_IsAvailableChanged)
		bool IsAvailable = true;;
	
	FTimerHandle RespawnTimerHandle;



	/// Methods

public:
	APickupBase();
	//bool CanInteract() const { return bExplicitInteraction && IsAvailable; }
	

	virtual bool CanInteract(IAffectableInterface* const Affectable, OUT float& OutDelay)
	{
		OutDelay = 0;
		return  bExplicitInteraction && IsAvailable;
	}
	bool AuthTryInteract(IAffectableInterface* const Affectable);

protected:

	// Override this to do whatever.
	virtual bool TryApplyAffect(IAffectableInterface* const Affectable)
	{
		unimplemented(); return false;
	}

private:
	UFUNCTION()
		void OnCompBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
		void OnRep_IsAvailableChanged();
	
	bool TryPickup(IAffectableInterface* Affectable);
	void PickupItem();
	void Respawn();
	void MakePickupAvailable(bool bIsAvailable);

	void LogMsgWithRole(FString message);
	FString GetEnumText(ENetRole role);
	FString GetRoleText();
};
