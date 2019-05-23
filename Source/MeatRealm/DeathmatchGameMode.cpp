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
	/*const auto Hero = static_cast<AHeroController*>(NewPlayer);
	ConnectedHeroControllers.AddUnique(Hero);

	const FString s = "ConnectedHeroControllers:" + ConnectedHeroControllers.Num();
	UE_LOG(LogTemp, Warning, TEXT("%s"), *s);*/
}
//
//void ADeathmatchGameMode::Logout(AController* Exiting)
//{
//	/*const auto Hero = static_cast<AHeroController*>(Exiting);
//	ConnectedHeroControllers.Remove(Hero);
//
//	const FString s = "ConnectedHeroControllers:" + ConnectedHeroControllers.Num();
//	UE_LOG(LogTemp, Warning, TEXT("%s"), *s);*/
//}
