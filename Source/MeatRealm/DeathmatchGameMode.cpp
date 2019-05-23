// Fill out your copyright notice in the Description page of Project Settings.


#include "DeathmatchGameMode.h"
#include "DeathmatchGameState.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "HeroCharacter.h"
#include "HeroState.h"
#include "Engine/World.h"

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
	BindEvents(Hero);

	ConnectedHeroControllers.AddUnique(Hero);

	UE_LOG(LogTemp, Warning, TEXT("ConnectedHeroControllers: %d"), ConnectedHeroControllers.Num());
}

void ADeathmatchGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	const auto Hero = static_cast<AHeroController*>(Exiting);
	UnbindEvents(Hero);

	ConnectedHeroControllers.Remove(Hero);

	UE_LOG(LogTemp, Warning, TEXT("ConnectedHeroControllers: %d"), ConnectedHeroControllers.Num());
}

bool ADeathmatchGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	// Always pick a random spawn
	return false;
}

void ADeathmatchGameMode::BindEvents(AHeroController* Controller)
{
	FDelegateHandle Handle = Controller->GetHeroCharacter()->OnHealthDepleted().AddUObject(this, &ADeathmatchGameMode::OnPlayerDie);
}

void ADeathmatchGameMode::UnbindEvents(AHeroController* Controller)
{
	//Controller->GetHeroCharacter()->OnHealthDepleted().Remove(Handle)
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

	Controller->ServerRestartPlayer();
	
	auto loc = FVector{0,0,500};
	FTransform tform{ loc };

	auto heroChar = Controller->GetHeroCharacter();
	auto heroState = heroChar->GetHeroState();

	// HACK! TODO Is this event leaking? This is bad. Maybe put the event on the HeroState along with HP. Let the HeroCharacter be a dumb container. :D
	heroChar->OnHealthDepleted().AddUObject(this, &ADeathmatchGameMode::OnPlayerDie);

	UE_LOG(LogTemp, Warning, TEXT("Respawned guy: %fhp %dk %dd"), heroChar->Health, heroState->Kills, heroState->Deaths);

}

