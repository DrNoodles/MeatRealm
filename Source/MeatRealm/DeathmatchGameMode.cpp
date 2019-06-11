// Fill out your copyright notice in the Description page of Project Settings.


#include "DeathmatchGameMode.h"
#include "DeathmatchGameState.h"
#include "UObject/ConstructorHelpers.h"
#include "HeroCharacter.h"
#include "HeroState.h"
#include "ScoreboardEntryData.h"
#include "HeroController.h"
#include "Projectile.h"
#include "KillfeedEntryData.h"

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
	
	// TODO Use GameState->PlayerState->PlayerId 
	// https://api.unrealengine.com/INT/API/Runtime/Engine/GameFramework/APlayerState/PlayerId/index.html
	const uint32 UID = Hero->GetUniqueID();

	ConnectedHeroControllers.Add(UID, Hero); 
	UE_LOG(LogTemp, Warning, TEXT("ConnectedHeroControllers: %d"), ConnectedHeroControllers.Num());

	// Monitor for player death events
	///*const FDelegateHandle Handle = */Hero->OnTakenDamage.AddDynamic(this, &ADeathmatchGameMode::OnPlayerTakeDamage);
	//OnPlayerDieHandles.Add(UID, Handle);
}


void ADeathmatchGameMode::Logout(AController* Exiting)
{
	const auto Hero = static_cast<AHeroController*>(Exiting);

	ConnectedHeroControllers.Remove(Hero->GetUniqueID());
	UE_LOG(LogTemp, Warning, TEXT("ConnectedHeroControllers: %d"), ConnectedHeroControllers.Num());

//Hero->OnTakenDamage.RemoveDynamic(this, &ADeathmatchGameMode::OnPlayerTakeDamage);
	// Unbind event when player leaves!
	//const uint32 UID = Hero->GetUniqueID();
	//const auto Handle = OnPlayerDieHandles[UID];
	/*Hero->OnTakenDamage().Remove(Handle);*/

	Super::Logout(Exiting);
}


bool ADeathmatchGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return false; // Always pick a random spawn
}

void ADeathmatchGameMode::OnPlayerTakeDamage(FMRHitResult Hit)
{
	if (!HasAuthority()) return;

	//UE_LOG(LogTemp, Warning, TEXT("TakeDamage %d"), Hit.DamageTaken);

	if (!ConnectedHeroControllers.Contains(Hit.ReceiverControllerId))
	{
		UE_LOG(LogTemp, Error, TEXT("ADeathmatchGameMode::OnPlayerTakeDamage cant find receiver controller!"));
		return;
	}
	if (!ConnectedHeroControllers.Contains(Hit.AttackerControllerId))
	{
		UE_LOG(LogTemp, Error, TEXT("ADeathmatchGameMode::OnPlayerTakeDamage cant find attacker controller!"));
		return;
	}

	const auto AttackerController = ConnectedHeroControllers[Hit.AttackerControllerId];
	// TODO Route hit location through OnPlayerTakeDamage. Probs time to introduce a hit struct!
	
	AttackerController->SimulateHitGiven(Hit);


	const auto ReceivingController = ConnectedHeroControllers[Hit.ReceiverControllerId];
	//ReceivingController->HitTaken(Hit);

	

	if (Hit.HealthRemaining <= 0)
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

		AddKillfeedEntry(AttackerController, ReceivingController);

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


void ADeathmatchGameMode::AddKillfeedEntry(AHeroController* const Killer, AHeroController* const Dead)
{
	FString KillerName{}, DeadName{};

	if (Killer) KillerName = Killer->PlayerState->GetPlayerName(); // will this crash if they've left?
	if (Dead) DeadName = Dead->PlayerState->GetPlayerName();

	auto*const GS = GetGameState<ADeathmatchGameState>();
	if (GS)
	{
		GS->AddKillfeedData(KillerName, "killed", DeadName);
	}
}

