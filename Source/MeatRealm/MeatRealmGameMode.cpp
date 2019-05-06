// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "MeatRealmGameMode.h"
#include "MeatRealmPawn.h"

AMeatRealmGameMode::AMeatRealmGameMode()
{
	// set default pawn class to our character class
	DefaultPawnClass = AMeatRealmPawn::StaticClass();
}

