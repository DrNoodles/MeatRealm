// Fill out your copyright notice in the Description page of Project Settings.


#include "HeroController.h"
#include "HeroCharacter.h"

void AHeroController::OnPossess(APawn* InPawn)
{
	// Called on server upon possessing a pawn

	LogMsgWithRole("AHeroController::OnPossess()");
	Super::OnPossess(InPawn);
}

void AHeroController::AcknowledgePossession(APawn* P)
{
	// Called on owning-client upon possessing a pawn

	LogMsgWithRole("AHeroController::AcknowledgePossession()");
	Super::AcknowledgePossession(P);
}

void AHeroController::OnUnPossess()
{
	// Called on server and owning-client upon depossessing a pawn

	LogMsgWithRole("AHeroController::OnUnPossess()");
	Super::OnUnPossess();
}

AHeroCharacter* AHeroController::GetHeroCharacter() const
{
	const auto Character = GetCharacter();
	return Character == nullptr ? nullptr : (AHeroCharacter*)Character;
}

void AHeroController::ShowHud(bool bMakeVisible)
{
	const bool bOwningClient = GetLocalRole() == ROLE_AutonomousProxy;
	const bool bListenServer = GetRemoteRole() == ROLE_SimulatedProxy;
	if (!bOwningClient && !bListenServer) 
	{
		return;
	}

	if (!HudClass)
	{
		UE_LOG(LogTemp, Error, TEXT("HeroControllerBP: Must set HUD class in BP to display it.")) 
		return;
	};


	// If the hud exists, remove it!
	if (HudInstance != nullptr)
	{
		HudInstance->RemoveFromParent();
		HudInstance = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Destroyed HUD"));
	}

	if (bMakeVisible)
	{
		// Create and attach a new hud
		HudInstance = CreateWidget<UUserWidget>(this, HudClass);
		if (HudInstance != nullptr)
		{
			HudInstance->AddToViewport();
			UE_LOG(LogTemp, Warning, TEXT("Created HUD"));
		}
	}
}




void AHeroController::LogMsgWithRole(FString message)
{
	FString m = GetRoleText() + ": " + message;
	UE_LOG(LogTemp, Warning, TEXT("%s"), *m);
}
FString AHeroController::GetEnumText(ENetRole role)
{
	switch (role) {
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "SimulatedProxy";
	case ROLE_AutonomousProxy:
		return "AutonomouseProxy";
	case ROLE_Authority:
		return "Authority";
	case ROLE_MAX:
	default:
		return "ERROR";
	}
}
FString AHeroController::GetRoleText()
{
	auto Local = Role;
	auto Remote = GetRemoteRole();


	if (Remote == ROLE_SimulatedProxy) //&& Local == ROLE_Authority
		return "ListenServer";

	if (Local == ROLE_Authority)
		return "Server";

	if (Local == ROLE_AutonomousProxy) // && Remote == ROLE_Authority
		return "OwningClient";

	if (Local == ROLE_SimulatedProxy) // && Remote == ROLE_Authority
		return "SimClient";

	return "Unknown: " + GetEnumText(Role) + " " + GetEnumText(GetRemoteRole());
}

