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

	auto Char = GetHeroCharacter();
	if (Char) Char->SetUseMouseAim(bShowMouseCursor);
}

void AHeroController::OnUnPossess()
{
	// Called on server and owning-client upon depossessing a pawn

	LogMsgWithRole("AHeroController::OnUnPossess()");
	Super::OnUnPossess();
}

AHeroCharacter* AHeroController::GetHeroCharacter() const
{
	const auto Char = GetCharacter();
	return Char == nullptr ? nullptr : (AHeroCharacter*)Char;
}

void AHeroController::ShowHud(bool bMakeVisible)
{
	// Enforce vertical aspect ratio
	const auto LP = GetLocalPlayer();
	LP->AspectRatioAxisConstraint = EAspectRatioAxisConstraint::AspectRatio_MaintainYFOV;

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

void AHeroController::HealthDepleted(AHeroController* DamageInstigator)
{
	HealthDepletedEvent.Broadcast(this, DamageInstigator);
}

void AHeroController::SetupInputComponent()
{
	Super::SetupInputComponent();
	auto I = InputComponent;
	I->BindAxis("MoveUp", this, &AHeroController::Input_MoveUp);
	I->BindAxis("MoveRight", this, &AHeroController::Input_MoveRight);
	I->BindAxis("FaceUp", this, &AHeroController::Input_FaceUp);
	I->BindAxis("FaceRight", this, &AHeroController::Input_FaceRight);
	I->BindAction("FireWeapon", IE_Pressed, this, &AHeroController::Input_FirePressed);
	I->BindAction("FireWeapon", IE_Released, this, &AHeroController::Input_FireReleased);
	I->BindAction("Reload", IE_Released, this, &AHeroController::Input_Reload);
	I->BindAction("ToggleInputScheme", IE_Pressed, this, &AHeroController::Input_ToggleUseMouse);
}


/// Input

void AHeroController::Input_MoveUp(float Value)
{
	auto Char = GetHeroCharacter();
	if (Char) Char->Input_MoveUp(Value);
}

void AHeroController::Input_MoveRight(float Value)
{
	auto Char = GetHeroCharacter();
	if (Char) Char->Input_MoveRight(Value);
}

void AHeroController::Input_FaceUp(float Value)
{
	auto Char = GetHeroCharacter();
	if (Char) Char->Input_FaceUp(Value);
}

void AHeroController::Input_FaceRight(float Value)
{
	auto Char = GetHeroCharacter();
	if (Char) Char->Input_FaceRight(Value);
}

void AHeroController::Input_FirePressed()
{
	auto Char = GetHeroCharacter();
	if (Char) Char->Input_FirePressed();
}

void AHeroController::Input_FireReleased()
{
	auto Char = GetHeroCharacter();
	if (Char) Char->Input_FireReleased();
}

void AHeroController::Input_Reload()
{
	auto Char = GetHeroCharacter();
	if (Char) Char->Input_Reload();
}

void AHeroController::Input_ToggleUseMouse()
{
	bShowMouseCursor = !bShowMouseCursor;

	auto Char = GetHeroCharacter();
	if (Char) Char->SetUseMouseAim(bShowMouseCursor);
}

