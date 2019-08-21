// Fill out your copyright notice in the Description page of Project Settings.


#include "DeathmatchGameState.h"
#include "HeroState.h"
#include "ScoreboardEntryData.h"
#include "UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "KillfeedEntryData.h"

void ADeathmatchGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADeathmatchGameState, KillfeedData);
}

float ADeathmatchGameState::GetPlayerRespawnDelay(AController* Controller) const
{
	auto val = Super::GetPlayerRespawnDelay(Controller);
	FString str = FString::Printf(TEXT("got delay of % f seconds"), val);
	LogMsgWithRole(str);
	
	return val;
}

bool ADeathmatchGameState::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (auto* Item : KillfeedData)
	{
		if (Item != nullptr)
		{
			WroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

TArray<UScoreboardEntryData*> ADeathmatchGameState::GetScoreboard()
{
	TArray<UScoreboardEntryData*> Scoreboard{};

	for (APlayerState* PlayerState : PlayerArray)
	{
		const auto Hero = Cast<AHeroState>(PlayerState);
		if (Hero->HasLeftTheGame)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Player %s is inactive and excluded from the scoreboard."), *PlayerState->GetPlayerName());
			continue; // Player has left the game? // TODO Check for spectators?
		}

		UScoreboardEntryData* Item = NewObject<UScoreboardEntryData>();
		Item->Name = Hero->GetPlayerName();
		Item->Kills = Hero->Kills;
		Item->Deaths = Hero->Deaths;
		Item->Ping = Hero->Ping;

		Scoreboard.Add(Item);
	}

	Scoreboard.Sort([](const UScoreboardEntryData & A, const UScoreboardEntryData & B)
		{
			if (A.Kills == B.Kills)
			{
				return A.Deaths < B.Deaths;
			}
			
			return A.Kills > B.Kills;
		});

	return std::move(Scoreboard);
}

void ADeathmatchGameState::StartARemoveKillfeedItemTimer()
{
	// Create a timer
	FTimerHandle Handle;

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(
			Handle, this, &ADeathmatchGameState::FinishOldestKillfeedItemTimer, KillfeedItemDuration, false, -1);
		
		Timers.Add(Handle);
	}
}

void ADeathmatchGameState::FinishOldestKillfeedItemTimer()
{
	if (Timers.Num() == 0) 
		UE_LOG(LogTemp, Error, TEXT("Attempted to move a timer but there aren't any!"));

	if (KillfeedData.Num() == 0) 
		UE_LOG(LogTemp, Error, TEXT("Attempted to move a timer but there aren't any!"));

	auto FirstTimer = Timers[0];
	Timers.RemoveAt(0);
	KillfeedData.RemoveAt(0);

	UWorld* World = GetWorld();
	if (FirstTimer.IsValid() && World)
	{
		World->GetTimerManager().ClearTimer(FirstTimer);
	}
	
	// Make sure a listen server knows about this
	if (GetNetMode() != NM_DedicatedServer)
	{
		OnRep_KillfeedDataChanged();
	}
}

void ADeathmatchGameState::AddKillfeedData(const FString& Victor, const FString& Verb, const FString& Dead)
{
	if (!HasAuthority()) return;

	//LogMsgWithRole("ADeathmatchGameState::AddKillfeedData()");

	UKillfeedEntryData* Entry = NewObject<UKillfeedEntryData>(this);
	Entry->Winner = Victor;
	Entry->Verb = "killed";
	Entry->Loser = Dead;
	KillfeedData.Add(Entry);

	if (KillfeedData.Num() > 4)
	{
		FinishOldestKillfeedItemTimer();
	}

	StartARemoveKillfeedItemTimer();
	

	// Make sure a listen server knows about this
	if (GetNetMode() != NM_DedicatedServer)
	{
		OnRep_KillfeedDataChanged();
	}
}

void ADeathmatchGameState::OnRep_KillfeedDataChanged()
{
	OnKillfeedChanged.Broadcast();
	/*UE_LOG(LogTemp, Warning, TEXT("KILLFEED"));
	for (UKillfeedEntryData* entry : KillfeedData)
	{
		if (!entry)
		{
			UE_LOG(LogTemp, Warning, TEXT("  Entry is null"));
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("  Entry %d %s %s %s"), KillfeedData.Num(),
			*entry->Winner, *entry->Verb, *entry->Loser);
	}*/
}



void ADeathmatchGameState::NotifyIncomingSuper(float PowerUpAnnouncementLeadTime, const FString& LocationMsg)
{
	//LogMsgWithRole("ADeathmatchGameState::NotifyIncomingSuper");
	MultiNotifyIncomingSuper(PowerUpAnnouncementLeadTime, LocationMsg);
}

void ADeathmatchGameState::MultiNotifyIncomingSuper_Implementation(float PowerUpAnnouncementLeadTime,
	const FString& LocationMsg)
{
	OnIncomingSuper.Broadcast(PowerUpAnnouncementLeadTime, LocationMsg);
}


void ADeathmatchGameState::LogMsgWithRole(FString message) const
{
	FString m = GetRoleText() + ": " + message;
	UE_LOG(LogTemp, Warning, TEXT("%s"), *m);
}
FString ADeathmatchGameState::GetEnumText(ENetRole role) const
{
	switch (role) {
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "Sim";
	case ROLE_AutonomousProxy:
		return "Auto";
	case ROLE_Authority:
		return "Auth";
	case ROLE_MAX:
		return "MAX (error?)";
	default:
		return "ERROR";
	}
}
FString ADeathmatchGameState::GetRoleText() const
{
	return GetEnumText(Role) + " " + GetEnumText(GetRemoteRole()) + " Ded:" + (IsRunningDedicatedServer() ? "True" : "False");

}


