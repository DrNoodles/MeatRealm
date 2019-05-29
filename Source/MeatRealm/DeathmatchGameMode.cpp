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

	// Monitor for player death events
	const FDelegateHandle Handle = Hero->OnHealthDepleted().AddUObject(this, &ADeathmatchGameMode::OnPlayerDie);
	const uint32 UID = Hero->GetUniqueID();
	OnPlayerDieHandles.Add(UID, Handle);
}


void ADeathmatchGameMode::Logout(AController* Exiting)
{
	const auto Hero = static_cast<AHeroController*>(Exiting);

	ConnectedHeroControllers.Remove(Hero);
	UE_LOG(LogTemp, Warning, TEXT("ConnectedHeroControllers: %d"), ConnectedHeroControllers.Num());

	// Unbind event when player leaves!
	const uint32 UID = Hero->GetUniqueID();
	const auto Handle = OnPlayerDieHandles[UID];
	Hero->OnHealthDepleted().Remove(Handle);

	Super::Logout(Exiting);
}


bool ADeathmatchGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	// Always pick a random spawn
	return false;
}


void ADeathmatchGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	UE_LOG(LogTemp, Warning, TEXT("SetPlayerDefaults"));
}

void ADeathmatchGameMode::OnPlayerDie(AHeroController* DeadController, AHeroController* KillerController)
{
	if (!DeadController)
	{
		UE_LOG(LogTemp, Error, TEXT("ADeathmatchGameMode::OnPlayerDie dead is null!"));
		return;
	}
	if (!KillerController)
	{
		UE_LOG(LogTemp, Error, TEXT("ADeathmatchGameMode::OnPlayerDie killer is null!"));
		return;
	}

	if (!DeadController || !KillerController) return;

	AHeroState* const DeadState = DeadController->GetPlayerState<AHeroState>();
	AHeroState* const KillerState = KillerController->GetPlayerState<AHeroState>();

	DeadState->Deaths++;
	KillerState->Kills++;
	UE_LOG(LogTemp, Warning, TEXT("Deadguy: %dk:%dd"), DeadState->Kills, DeadState->Deaths);
	UE_LOG(LogTemp, Warning, TEXT("Killer: %dk:%dd"), KillerState->Kills, KillerState->Deaths);

	AHeroCharacter* DeadChar = DeadController->GetHeroCharacter();
	if (DeadChar) DeadChar->Destroy();

	if (EndGameIfFragLimitReached()) return;

	RestartPlayer(DeadController);
}



bool ADeathmatchGameMode::EndGameIfFragLimitReached() const
{
	auto DMGameState = GetGameState<ADeathmatchGameState>();
	auto Scores = DMGameState->GetScoreboard();
	
	const auto bFragLimitReached = Scores.Num() > 0 && Scores[0]->Kills >= DMGameState->FragLimit;
	if (bFragLimitReached)
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit Frag Limit!"));

		const auto World = GetWorld();
		if (World) World->ServerTravel("/Game/MeatRealm/Maps/TestMap");
		return true;
	}

	return false;
}


