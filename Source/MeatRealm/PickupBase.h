// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/AffectableInterface.h"

#include "PickupBase.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UCapsuleComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPickupSpawned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPickupTaken);

UCLASS()
class MEATREALM_API APickupBase : public AActor
{
	GENERATED_BODY()
	
public:	
	APickupBase();

	bool CanInteract() const { return bExplicitInteraction && IsAvailable; }
	bool TryInteract(IAffectableInterface* const Affectable);

protected:


	// Override this to do whatever.
	virtual bool TryApplyAffect(IAffectableInterface* const Affectable)
	{
		unimplemented(); return false;
	}


	bool TryPickup(IAffectableInterface* Affectable);
	UFUNCTION()
	void OnCompBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FPickupSpawned OnSpawn;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FPickupSpawned OnTaken;

	// In Seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float RespawnDelay = 20;


protected:	/// Components
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* MeshComp = nullptr;

	UPROPERTY(VisibleAnywhere)
		UCapsuleComponent* CollisionComp = nullptr;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRPC_PickupItem();

	UPROPERTY(ReplicatedUsing = OnRep_IsAvailableChanged)
		bool IsAvailable = true;;

	UPROPERTY(EditAnywhere)
		bool bExplicitInteraction = false;


	UFUNCTION()
		void OnRep_IsAvailableChanged();

	void LogMsgWithRole(FString message);
	FString GetEnumText(ENetRole role);
	FString GetRoleText();


	FTimerHandle RespawnTimerHandle;

private:
	void MakePickupAvailable(bool bIsAvailable);
	void Respawn();

};
