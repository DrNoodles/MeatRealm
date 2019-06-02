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
	const FDelegateHandle Handle = Hero->OnTakenDamage().AddUObject(this, &ADeathmatchGameMode::OnPlayerTakeDamage);
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
	Hero->OnTakenDamage().Remove(Handle);

	Super::Logout(Exiting);
}


bool ADeathmatchGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return false; // Always pick a random spawn
}

void ADeathmatchGameMode::OnPlayerTakeDamage(uint32 ReceiverControllerId, uint32 AttackerControllerId, int HealthRemaining, int DamageTaken, bool bHitArmour)
{
	UE_LOG(LogTemp, Warning, TEXT("TakeDamage"));

	if (!ConnectedHeroControllers.Contains(ReceiverControllerId))
	{
		UE_LOG(LogTemp, Error, TEXT("ADeathmatchGameMode::OnPlayerTakeDamage cant find receiver controller!"));
		return;
	}
	if (!ConnectedHeroControllers.Contains(AttackerControllerId))
	{
		UE_LOG(LogTemp, Error, TEXT("ADeathmatchGameMode::OnPlayerTakeDamage cant find attacker controller!"));
		return;
	}

	const auto AttackerController = ConnectedHeroControllers[AttackerControllerId];

	// 


	const auto ReceivingController = ConnectedHeroControllers[ReceiverControllerId];




	if (HealthRemaining <= 0)
	{
		// Award the killer a point
		if (AttackerController) AttackerController->GetPlayerState<AHeroState>()->Kills++;
		
		// Award death point, kill then respawn character
		if (ReceivingController)
		{
			ReceivingController->GetPlayerState<AHeroState>()->Deaths++;

			AHeroCharacter* DeadChar = ReceivingController->GetHeroCharacter();
			if (DeadChar) DeadChar->Destroy();

			RestartPlayer(ReceivingController);
		}


		if (EndGameIfFragLimitReached()) return;
	}
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


