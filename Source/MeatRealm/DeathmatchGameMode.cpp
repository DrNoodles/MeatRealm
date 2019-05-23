// Fill out your copyright notice in the Description page of Project Settings.


#include "DeathmatchGameMode.h"
#include "DeathmatchGameState.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"
#include "HeroCharacter.h"
#include "HeroState.h"

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
}
//
//void ADeathmatchGameMode::BeginPlay() 
//{
//	//auto state = (ADeathmatchGameState*)GameState;
//
//	
//}
//
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

void ADeathmatchGameMode::OnPlayerDie(AHeroCharacter* dead, AHeroCharacter* killer)
{
	UE_LOG(LogTemp, Warning, TEXT("Player ded"));
}

void ADeathmatchGameMode::Blah()
{
	UE_LOG(LogTemp, Warning, TEXT("Player ded"));
}

void ADeathmatchGameMode::BindEvents(AHeroController* c)
{
	auto ch = c->GetCharacter();
	if (ch == nullptr) return;

	auto hero = (AHeroCharacter*)ch;
	hero->OnHealthDepleted().AddUObject(this, &ADeathmatchGameMode::OnPlayerDie);
}

void ADeathmatchGameMode::UnbindEvents(AHeroController* c)
{
	auto ch = c->GetCharacter();
	if (ch == nullptr) return;

	auto hero = (AHeroCharacter*)ch;
	//hero->NoHealthRemains.Unbind();
}
