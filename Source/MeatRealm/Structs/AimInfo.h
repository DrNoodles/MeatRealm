#pragma once


#include "CoreMinimal.h"

#include "AimInfo.generated.h"

USTRUCT()
struct FAimInfo
{
	GENERATED_BODY()

	// Transform at the end of the 'barrel'
	FTransform Muzzle;

	// The direction the pawn is facing as a result of aiming
	FVector PawnLookVec;

	// The effective aim point projected onto an upright plane (n=(0,0,1)) containing the muzzle location
	FVector CursorOnAimPlane;

	// The pawns location projected onto an upright plane (n=(0,0,1)) containing the muzzle location
	FVector PawnLocationOnAimPlane;
	
};

