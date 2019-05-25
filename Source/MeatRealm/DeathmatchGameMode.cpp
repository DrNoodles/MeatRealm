// Fill out your copyright notice in the Description page of Project Settings.


#include "DeathmatchGameMode.h"
#include "DeathmatchGameState.h"
#include "UObject/ConstructorHelpers.h"
#include "HeroCharacter.h"
#include "HeroState.h"
#include "ScoreboardEntryData.h"

ADeathmatchGameMode::ADeathmatchGameMode()
{
	const ConstructorHelpers::FClassFinder<AHeroCharacter> PawnFinder(
		TEXT("/Game/MeatRealm/Character/HeroCharacterBP"));
	const ConstructorHelpers::FClassFinder<AHeroController> PlayerControllerFinder(
		TEXT("/Game/MeatRealm/Game/HeroControllerBP"));

	DefaultPlayerName = FText::FromString("Sasquatch");
	DefaultPawnClass = PawnFinder.Class;
	PlayerControllerClass = PlayerControllerFinder.Class;
	PlayerStateClass = AHeroState::StaticClass();
	GameStateClass = ADeathmatchGameState::StaticClass();

	bStartPlayersAsSpectators = false;
}


void ADeathmatchGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	const auto Hero = static_cast<AHeroController*>(NewPlayer);
	ConnectedHeroControllers.AddUnique(Hero);

	UE_LOG(LogTemp, Warning, TEXT("ConnectedHeroControllers: %d"), ConnectedHeroControllers.Num());
}


void ADeathmatchGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	const auto Hero = static_cast<AHeroController*>(Exiting);
	ConnectedHeroControllers.Remove(Hero);

	UE_LOG(LogTemp, Warning, TEXT("ConnectedHeroControllers: %d"), ConnectedHeroControllers.Num());
}


bool ADeathmatchGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	// Always pick a random spawn
	return false;
}


void ADeathmatchGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	UE_LOG(LogTemp, Warning, TEXT("SetPlayerDefaults"));
	
	auto heroChar = (AHeroCharacter*)PlayerPawn;
	heroChar->Health = 100;
	heroChar->OnHealthDepleted().AddUObject(this, &ADeathmatchGameMode::OnPlayerDie);
}

void ADeathmatchGameMode::OnPlayerDie(AHeroCharacter* dead, AHeroCharacter* killer)
{
	auto DeadState = dead->GetHeroState();
	auto KillerState = killer->GetHeroState();

	DeadState->Deaths++;
	KillerState->Kills++;
	UE_LOG(LogTemp, Warning, TEXT("Deadguy: %dk:%dd"), DeadState->Kills, DeadState->Deaths);
	UE_LOG(LogTemp, Warning, TEXT("Killer: %dk:%dd"), KillerState->Kills, KillerState->Deaths);

	AHeroController* Controller = dead->GetHeroController();
	dead->Destroy();


	RestartPlayer(Controller);

	EndGameIfFragLimitReached();
}

void ADeathmatchGameMode::EndGameIfFragLimitReached() const
{
	auto DMGameState = GetGameState<ADeathmatchGameState>();
	auto Scores = DMGameState->GetScoreboard();
	if (Scores.Num() > 0 && Scores[0]->Kills >= DMGameState->FragLimit)
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit Frag Limit!"));

		const auto World = GetWorld();
		if (World) World->ServerTravel("/Game/MeatRealm/Maps/TestMap");
	}
}


