// Fill out your copyright notice in the Description page of Project Settings.


#include "DeathmatchGameMode.h"
#include "DeathmatchGameState.h"
#include "UObject/ConstructorHelpers.h"
#include "HeroCharacter.h"
#include "HeroState.h"
#include "ScoreboardEntryData.h"
#include "HeroController.h"
#include "Projectile.h"

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
	const uint32 UID = Hero->GetUniqueID();

	ConnectedHeroControllers.Add(UID, Hero);
	UE_LOG(LogTemp, Warning, TEXT("ConnectedHeroControllers: %d"), ConnectedHeroControllers.Num());

	// Monitor for player death events
	const FDelegateHandle Handle = Hero->OnHealthDepleted().AddUObject(this, &ADeathmatchGameMode::OnPlayerDie);
	OnPlayerDieHandles.Add(UID, Handle);
}


void ADeathmatchGameMode::Logout(AController* Exiting)
{
	const auto Hero = static_cast<AHeroController*>(Exiting);

	ConnectedHeroControllers.Remove(Hero->GetUniqueID());
	UE_LOG(LogTemp, Warning, TEXT("ConnectedHeroControllers: %d"), ConnectedHeroControllers.Num());

	// Unbind event when player leaves!
	const uint32 UID = Hero->GetUniqueID();
	const auto Handle = OnPlayerDieHandles[UID];
	Hero->OnHealthDepleted().Remove(Handle);

	Super::Logout(Exiting);
}


bool ADeathmatchGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return false; // Always pick a random spawn
}

void ADeathmatchGameMode::OnPlayerDie(uint32 DeadControllerId, uint32 KillerControllerId)
{
	if (!ConnectedHeroControllers.Contains(DeadControllerId))
	{
		UE_LOG(LogTemp, Error, TEXT("ADeathmatchGameMode::OnPlayerDie cant find dead controller!"));
		return;
	}
	if (!ConnectedHeroControllers.Contains(KillerControllerId))
	{
		UE_LOG(LogTemp, Error, TEXT("ADeathmatchGameMode::OnPlayerDie cant find killer controller!"));
		return;
	}


	// Award the killer a point
	const auto KillerController = ConnectedHeroControllers[KillerControllerId];
	if (KillerController) KillerController->GetPlayerState<AHeroState>()->Kills++;


	// Award death point, kill then respawn character
	const auto DeadController = ConnectedHeroControllers[DeadControllerId];
	if (DeadController)
	{
		DeadController->GetPlayerState<AHeroState>()->Deaths++;

		AHeroCharacter* DeadChar = DeadController->GetHeroCharacter();
		if (DeadChar) DeadChar->Destroy();

		RestartPlayer(DeadController);
	}

	
	if (EndGameIfFragLimitReached()) return;
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


