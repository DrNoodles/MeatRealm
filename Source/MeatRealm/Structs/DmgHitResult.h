#pragma once


#include "CoreMinimal.h"
#include "DmgHitResult.generated.h"

// TODO Use something built in already?
USTRUCT(BlueprintType)
struct MEATREALM_API FMRHitResult
{
	GENERATED_BODY()

	UPROPERTY()
		uint32 ReceiverControllerId;
	UPROPERTY()
		uint32 AttackerControllerId;
	UPROPERTY()
		int HealthRemaining;
	UPROPERTY()
		int DamageTaken;
	UPROPERTY()
		bool bHitArmour;
	UPROPERTY()
		FVector HitLocation;
	UPROPERTY()
		FVector HitDirection;
};

