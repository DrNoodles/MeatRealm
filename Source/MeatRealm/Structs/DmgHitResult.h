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
	UPROPERTY(BlueprintReadOnly)
		int HealthRemaining;
	UPROPERTY(BlueprintReadOnly)
		int DamageTaken;
	UPROPERTY(BlueprintReadOnly)
		bool bHitArmour;
	UPROPERTY(BlueprintReadOnly)
		FVector HitLocation;
	UPROPERTY(BlueprintReadOnly)
		FVector HitDirection;
};

