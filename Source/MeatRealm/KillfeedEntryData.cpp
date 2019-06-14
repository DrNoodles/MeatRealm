// Fill out your copyright notice in the Description page of Project Settings.


#include "KillfeedEntryData.h"
#include "UnrealNetwork.h"

void UKillfeedEntryData::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	DOREPLIFETIME(UKillfeedEntryData, Winner);
	DOREPLIFETIME(UKillfeedEntryData, Verb);
	DOREPLIFETIME(UKillfeedEntryData, Loser);
}
