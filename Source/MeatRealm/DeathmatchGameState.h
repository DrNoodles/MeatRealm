// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "KillfeedEntryData.h"

#include "DeathmatchGameState.generated.h"


class UScoreboardEntryData;
class UKillfeedEntryData;

UCLASS()
class MEATREALM_API ADeathmatchGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ADeathmatchGameState()
	{
		/*UKillfeedEntryData* Entry = NewObject<UKillfeedEntryData>();
		Entry->Winner = "W1";
		Entry->Verb = "V1";
		Entry->Loser = "L1";

		UKillfeedEntryData* Entry2 = NewObject<UKillfeedEntryData>();
		Entry->Winner = "W2";
		Entry->Verb = "V2";
		Entry->Loser = "L2";

		KillfeedData.Add(Entry);
		KillfeedData.Add(Entry2);*/
	}

	UFUNCTION(BlueprintCallable)
		TArray<UScoreboardEntryData*> GetScoreboard();

	/*UFUNCTION(BlueprintCallable)
		TArray<UKillfeedEntryData*> GetKillfeed();*/

	void AddKillfeedData(const FString& Victor, const FString& Verb, const FString& Dead);


	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int FragLimit = 15;

	// In Minutes
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float TimeLimit = 10;

	UPROPERTY(ReplicatedUsing=OnRep_TotalKills)
		int TotalKills = 0;
	UFUNCTION()
		void OnRep_TotalKills();

	//UPROPERTY(ReplicatedUsing = OnRep_KillfeedDataUpdated)
	//	TArray<UKillfeedEntryData*> KillfeedData{};

	//UFUNCTION()
	//void OnRep_KillfeedDataUpdated();

private:

	void LogMsgWithRole(FString message) const;
	FString GetEnumText(ENetRole role) const;
	FString GetRoleText() const;

};

