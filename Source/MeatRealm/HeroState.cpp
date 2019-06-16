// Fill out your copyright notice in the Description page of Project Settings.


#include "HeroState.h"
#include "UnrealNetwork.h"

void AHeroState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHeroState, Kills);
	DOREPLIFETIME(AHeroState, Deaths);
	DOREPLIFETIME(AHeroState, HasLeftTheGame);
}
