// Fill out your copyright notice in the Description page of Project Settings.


#include "DeathmatchGameState.h"
#include "HeroState.h"
#include "ScoreboardEntryData.h"

TArray<UScoreboardEntryData*> ADeathmatchGameState::GetScoreboard()
{
	TArray<UScoreboardEntryData*> Scoreboard{};

	for (APlayerState* PlayerState : PlayerArray)
	{
		const auto Hero = (AHeroState*)PlayerState;

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
