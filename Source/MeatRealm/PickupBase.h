// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/AffectableInterface.h"

#include "PickupBase.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UCapsuleComponent;

UCLASS()
class MEATREALM_API APickupBase : public AActor
{
	GENERATED_BODY()
	
public:	
	APickupBase();

	bool TryInteract(IAffectableInterface* const Affectable);

	bool GetExplicitInteraction() const { return bExplicitInteraction; }
	void SetExplicitInteraction(bool bIsExplicit);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bRespawns = true;

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

	
	UFUNCTION()
		void OnRep_IsAvailableChanged();

	void MakePickupAvailable(bool bIsAvailable);

	void Respawn();
	void LogMsgWithRole(FString message);
	FString GetEnumText(ENetRole role);
	FString GetRoleText();


	FTimerHandle RespawnTimerHandle;

private:
	UPROPERTY(EditAnywhere)
		bool bExplicitInteraction = false;
};
