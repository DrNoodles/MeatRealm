// Fill out your copyright notice in the Description page of Project Settings.


#include "KillfeedEntryData.h"

void UKillfeedEntryData::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	/*
	 DOREPLIFETIME(UKillfeedEntryData, Winner);
	DOREPLIFETIME(UKillfeedEntryData, Verb);
	DOREPLIFETIME(UKillfeedEntryData, Loser);
	*/

	DOREPLIFETIME(UKillfeedEntryData, TotalKills);

}

void UKillfeedEntryData::OnRep_Fart()
{
	UE_LOG(LogTemp, Warning, TEXT("Ass and titties. %d"), TotalKills);
}
