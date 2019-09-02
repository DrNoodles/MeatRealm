#pragma once


#include "CoreMinimal.h"
#include "DmgHitResult.generated.h"

// TODO Use something built in already?
USTRUCT(BlueprintType)
struct FMRHitResult
{
	GENERATED_BODY()

	UPROPERTY()
		uint32 VictimId;
	UPROPERTY()
		uint32 AttackerId;
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

