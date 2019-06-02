// Fill out your copyright notice in the Description page of Project Settings.


#include "HeroController.h"
#include "HeroCharacter.h"
#include "MRLocalPlayer.h"
#include "Engine/Public/DrawDebugHelpers.h"


AHeroController::AHeroController()
{

}

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


	if (IsLocalController())
	{
		ShowHud(true);
	}
}

void AHeroController::OnUnPossess()
{
	// Called on server and owning-client upon depossessing a pawn

	LogMsgWithRole("AHeroController::OnUnPossess()");

	if (IsLocalController())
	{
		ShowHud(false);
	}

	Super::OnUnPossess();
}

AHeroCharacter* AHeroController::GetHeroCharacter() const
{
	const auto Char = GetCharacter();
	return Char == nullptr ? nullptr : (AHeroCharacter*)Char;
}

void AHeroController::ShowHud(bool bMakeVisible)
{
	if (!IsLocalController()) return;

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


void AHeroController::DamageTaken(uint32 InstigatorHeroControllerId, float HealthRemaining, int DamageTaken, bool bHitArmour) const
{
	TakenDamageEvent.Broadcast(GetUniqueID(), InstigatorHeroControllerId, (int)HealthRemaining, DamageTaken, bHitArmour);
}

void AHeroController::SimulateHitGiven(const FMRHitResult& Hit)
{
	if (!IsLocalController())
	{
		//UE_LOG(LogTemp, Warning, TEXT("HitGiven() - NotLocal. Damage(%d)"), Hit.DamageTaken);
		ClientRPC_PlayHit(Hit);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("HitGiven() - Local. Damage(%d)"), Hit.DamageTaken);

	// Display a hit marker in the world
	auto World = GetWorld();
	if (World)
	{
		DrawDebugString(World, 
			Hit.HitLocation + FVector{ 0,0,100 },
			FString::FromInt(Hit.DamageTaken), 
			nullptr,
			Hit.bHitArmour ? FColor::Blue : FColor::White,
			0.5);
	}
}

void AHeroController::ClientRPC_PlayHit_Implementation(const FMRHitResult& Hit)
{
	SimulateHitGiven(Hit);
	//UE_LOG(LogTemp, Warning, TEXT("HitGiven() - Local. Damage(%d)"), Hit.DamageTaken);

	//// Display a hit marker in the world
	//auto World = GetWorld();
	//if (World)
	//{
	//	DrawDebugString(World,
	//		Hit.HitLocation + FVector{ 0,0,100 },
	//		FString::FromInt(Hit.DamageTaken),
	//		nullptr,
	//		Hit.bHitArmour ? FColor::Blue : FColor::White,
	//		0.5);
	//}
}

/// Input

void AHeroController::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AHeroController::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AHeroController::BeginPlay()
{
	Super::BeginPlay();
	// TODO Setup MRLocalPlayer in some kind of init function (BeginPlay?)
	//UPlayer* MRLocalPlayer = NewObject<UMRLocalPlayer>();
	//SetPlayer(MRLocalPlayer);

	// Enforce vertical aspect ratio
	const auto LP = GetLocalPlayer();
	if (LP && IsLocalController())
	{
		LogMsgWithRole("LP Get");
		LP->AspectRatioAxisConstraint = EAspectRatioAxisConstraint::AspectRatio_MaintainYFOV;
	}
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
	I->BindAction("Interact", IE_Pressed, this, &AHeroController::Input_Interact);
}

bool AHeroController::InputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	bool ret = Super::InputAxis(Key, Delta, DeltaTime, NumSamples, bGamepad);
	if (IsLocalController()) { SetUseMouseaim(!bGamepad); }
	return ret;
}

bool AHeroController::InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
	bool ret = Super::InputKey(Key, EventType, AmountDepressed, bGamepad);
	if (IsLocalController()) {	SetUseMouseaim(!bGamepad); }
	return ret;
}

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

void AHeroController::Input_Interact()
{
	auto Char = GetHeroCharacter();
	if (Char) Char->Input_Interact();
}

void AHeroController::SetUseMouseaim(bool bUseMouseAim)
{
	if (bUseMouseAim == bShowMouseCursor) return;

	bShowMouseCursor = bUseMouseAim;
	
	if (!bShowMouseCursor)
	{
		// HACK: Setting bShowMouseCursor doesn't hide the cursor till there's another input on the mouse.
		// So lets force that to happen so the cursor disappears nowS
		SetMouseLocation(0, 0);
	}

	auto Char = GetHeroCharacter();
	if (Char) Char->SetUseMouseAim(bUseMouseAim);
}





// Helpers

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
