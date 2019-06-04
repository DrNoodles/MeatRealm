// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "KillfeedEntryData.h"

#include "DeathmatchGameState.generated.h"


class UScoreboardEntryData;
class UKillfeedEntryData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKillfeedChanged);

UCLASS()
class MEATREALM_API ADeathmatchGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void PostInitializeComponents() override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	UFUNCTION(BlueprintCallable)
		TArray<UScoreboardEntryData*> GetScoreboard();

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FKillfeedChanged OnKillfeedChanged;

	void AddKillfeedData(const FString& Victor, const FString& Verb, const FString& Dead);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int FragLimit = 15;

	// In Minutes
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float TimeLimit = 10;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_KillfeedDataChanged)
		TArray<UKillfeedEntryData*> KillfeedData;
	UFUNCTION()
		void OnRep_KillfeedDataChanged() const;

private:
	bool IsClientControllingServerOwnedActor() const;

	void LogMsgWithRole(FString message) const;
	FString GetEnumText(ENetRole role) const;
	FString GetRoleText() const;
};

