// Fill out your copyright notice in the Description page of Project Settings.


#include "HeroController.h"
#include "HeroCharacter.h"

AHeroController::AHeroController()
{
	// TODO Get specific class of HUD. Then try set MyCharacter variable manually.
}

void AHeroController::BeginPlay()
{
	ShowHud(true);
}

AHeroCharacter* AHeroController::GetHeroCharacter() const
{
	return (AHeroCharacter*)GetCharacter();
}

void AHeroController::ShowHud(bool bMakeVisible)
{
	bool bOwningClient = GetLocalRole() == ROLE_AutonomousProxy;
	bool bListenServer = GetRemoteRole() == ROLE_SimulatedProxy;
	
	if (!bOwningClient && !bListenServer) return;

	//if (IsRunningDedicatedServer()) return; // HeroController can only be on dedicated server or owning client/listen server

	if (!wMainMenu) return;
	// TODO asset this!

	if (bMakeVisible)
	{

		// Create Hud Widget
		if (MyMainMenu == nullptr)
		{
			MyMainMenu = CreateWidget<UUserWidget>(this, wMainMenu);
		}

		// Attach to Viewport
		if (MyMainMenu != nullptr)
		{
			MyMainMenu->AddToViewport();
			UE_LOG(LogTemp, Warning, TEXT("Created HUD"));
		}
	}
	else
	{
		if (MyMainMenu != nullptr)
		{
			MyMainMenu->RemoveFromParent();
			MyMainMenu = nullptr;

			UE_LOG(LogTemp, Warning, TEXT("Destroyed HUD"));
		}
	}
}
