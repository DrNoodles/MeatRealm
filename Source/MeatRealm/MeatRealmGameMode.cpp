// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "MeatRealmGameMode.h"
#include "MeatRealmCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMeatRealmGameMode::AMeatRealmGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
