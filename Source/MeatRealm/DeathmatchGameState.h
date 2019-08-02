// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"

#include "DeathmatchGameState.generated.h"


class UScoreboardEntryData;
class UKillfeedEntryData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIncomingSuper, float, Time, FString, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FKillfeedChanged);

UCLASS()
class MEATREALM_API ADeathmatchGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void PostInitializeComponents() override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	UFUNCTION(BlueprintCallable)
		TArray<UScoreboardEntryData*> GetScoreboard();

	UFUNCTION(Client, Reliable)
	void ClientNotifyIncomingSuper(float PowerUpAnnouncementLeadTime, const FString& LocationMsg);
	
	void NotifyIncomingSuper(float PowerUpAnnouncementLeadTime, const FString& LocationMsg);

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FKillfeedChanged OnKillfeedChanged;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FIncomingSuper OnIncomingSuper;

	void StartARemoveTimer();
	void FinishOldestTimer();

	void AddKillfeedData(const FString& Victor, const FString& Verb, const FString& Dead);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int FragLimit = 15;

	// In Minutes
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float TimeLimit = 10;

	// How long (in seconds) a killfeed item is on screen
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float KillfeedItemDuration = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_KillfeedDataChanged)
		TArray<UKillfeedEntryData*> KillfeedData;

	UFUNCTION()
		void OnRep_KillfeedDataChanged();

private:
	TArray<FTimerHandle> Timers{};

	void LogMsgWithRole(FString message) const;
	FString GetEnumText(ENetRole role) const;
	FString GetRoleText() const;
};

