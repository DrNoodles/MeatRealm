// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "HeroCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"

#include "GameFramework/SpringArmComponent.h"
#include "Engine/Public/DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "UnrealNetwork.h"
#include "HeroState.h"
#include "HeroController.h"
#include "WeaponPickupBase.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"

/// Lifecycle

AHeroCharacter::AHeroCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// Replication
	bAlwaysRelevant = true;

	JumpMaxCount = 0;

	// Configure character movement
	GetCharacterMovement()->MaxWalkSpeed = 450;
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character move independently of facing
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bAbsoluteRotation = true; // Don't rotate with the character
	CameraBoom->RelativeRotation = FRotator(-56.f, 0.f, 0.f);
	CameraBoom->TargetArmLength = 1500;
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level
	CameraBoom->bEnableCameraLag = true;

	// Create a follow camera offset node
	FollowCameraOffsetComp = CreateDefaultSubobject<USceneComponent>(TEXT("FollowCameraOffset"));
	FollowCameraOffsetComp->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach 

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(FollowCameraOffsetComp); 
	FollowCamera->bAbsoluteRotation = false;
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
	FollowCamera->SetFieldOfView(38);

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	WeaponAnchor = CreateDefaultSubobject<UArrowComponent>(TEXT("WeaponAnchor"));
	WeaponAnchor->SetupAttachment(RootComponent);

	// Create TEMP aim pos comp to help visualise aiming target
	AimPosComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AimPosComp"));
	AimPosComp->SetupAttachment(RootComponent);
}

void AHeroCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ROLE_Authority == Role && CurrentWeapon != nullptr)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}
}

void AHeroCharacter::Restart()
{
	Super::Restart();
	LogMsgWithRole("AHeroCharacter::Restart()");

	Health = MaxHealth;
	Armour = 0.f;

	// Randomly select a weapon
	if (WeaponClasses.Num() > 0)
	{
		if (HasAuthority())
		{
			const auto Choice = FMath::RandRange(0, WeaponClasses.Num() - 1);
			ServerRPC_SpawnWeapon(WeaponClasses[Choice]);
		}
	}
}

void AHeroCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHeroCharacter, CurrentWeapon);
	DOREPLIFETIME(AHeroCharacter, Health);
	DOREPLIFETIME(AHeroCharacter, Armour);
}


AHeroState* AHeroCharacter::GetHeroState() const
{
	return GetPlayerState<AHeroState>();
}

AHeroController* AHeroCharacter::GetHeroController() const
{
	return GetController<AHeroController>();
}

void AHeroCharacter::ServerRPC_SpawnWeapon_Implementation(TSubclassOf<AWeapon> weaponClass)
{
	//LogMsgWithRole("AHeroCharacter::ServerRPC_SpawnWeapon");

	FActorSpawnParameters params;
	params.Instigator = this;
	params.Owner = this;

	auto weapon = GetWorld()->SpawnActorAbsolute<AWeapon>(
		weaponClass,
		WeaponAnchor->GetComponentTransform(), params);

	weapon->AttachToComponent(WeaponAnchor, FAttachmentTransformRules{ EAttachmentRule::KeepWorld, true });
	weapon->SetHeroControllerId(GetHeroController()->GetUniqueID());


	// Cleanup previous weapon
	if (CurrentWeapon != nullptr)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	// TODO Will this fuckup in flight projectiles

	// Make sure server has a copy
	CurrentWeapon = weapon;
}

bool AHeroCharacter::ServerRPC_SpawnWeapon_Validate(TSubclassOf<AWeapon> weaponClass)
{
	return true;
}


/// Methods

bool AHeroCharacter::IsClientControllingServerOwnedActor()
{
	return Role == ROLE_AutonomousProxy // Client on server
		|| HasAuthority() && !IsRunningDedicatedServer();
}

void AHeroCharacter::Tick(float DeltaSeconds)
{
	const auto HeroCont = GetHeroController();
	if (HeroCont == nullptr || !IsClientControllingServerOwnedActor()) return;



	// Handle Input (move and look)

	const auto deadzoneSquared = 0.25f * 0.25f;


	// Move character
	const auto moveVec = FVector{ AxisMoveUp, AxisMoveRight, 0 };
	if (moveVec.SizeSquared() >= deadzoneSquared)
	{
		AddMovementInput(FVector{ 1.f, 0.f, 0.f }, moveVec.X);
		AddMovementInput(FVector{ 0.f, 1.f, 0.f }, moveVec.Y);
	}


	// Calculate Look Vector for mouse or gamepad
	FVector LookVec;
	if (bUseMouseAim)
	{
		FVector WorldLocation, WorldDirection;
		const auto Success = HeroCont->DeprojectMousePositionToWorld(OUT WorldLocation, OUT WorldDirection);
		if (Success)
		{
			const FVector AnchorLoc = WeaponAnchor->GetComponentLocation();
			const FVector Hit = FMath::LinePlaneIntersection(
				WorldLocation, 
				WorldLocation + (WorldDirection * 5000), 
				AnchorLoc,
				FVector(0,0,1));

			LookVec = Hit - AnchorLoc;
		}
	}
	else // Use gamepad
	{
		LookVec = FVector{ AxisFaceUp, AxisFaceRight, 0 };
	}


	// Apply Look Vector - Aim character with look, if look is below deadzone then try use move vec
	if (LookVec.SizeSquared() >= deadzoneSquared)
	{
		Controller->SetControlRotation(LookVec.Rotation());
	}
	else if (moveVec.SizeSquared() >= deadzoneSquared)
	{
		Controller->SetControlRotation(moveVec.Rotation());
	}



	// Scan for interables in front of player

	auto* const Pickup = ScanForInteractable<AWeaponPickupBase>();
	if (Pickup && Pickup->CanInteract())
	{
		auto World = GetWorld();

		auto* InputSettings = UInputSettings::GetInputSettings();
		TArray<FInputActionKeyMapping> Mappings;
		InputSettings->GetActionMappingByName("Interact", OUT Mappings);

		auto ActionText = FText::FromString("Undefined");
		for (FInputActionKeyMapping Mapping : Mappings)
		{
			bool canUseKeyboardKey = bUseMouseAim && !Mapping.Key.IsGamepadKey();
			bool canUseGamepadKey = !bUseMouseAim && Mapping.Key.IsGamepadKey();

			if (canUseKeyboardKey || canUseGamepadKey)
			{
				ActionText = Mapping.Key.GetDisplayName();
				break;
			}
		}
		auto asdf = ActionText.ToString();
		if (World) DrawDebugString(World, Pickup->GetActorLocation() + FVector{ 0,0,200 }, "Equip (" + asdf + ")", nullptr, FColor::White, DeltaSeconds * 0.5);

		//LogMsgWithRole("Can Interact! ");
	}



	// Track camera with aim

	if (bLeanCameraWithAim)
	{
		FVector2D LinearLeanVector;
		
		if (bUseMouseAim)
		{
			if (bUseExperimentalMouseTracking)
			{
				ExperimentalMouseAimTracking(DeltaSeconds);
				return;
			}
			
			LinearLeanVector = TrackCameraWithAimMouse();
		}
		else
		{
			LinearLeanVector = TrackCameraWithAimGamepad();
		}

		const auto OffsetVec = LinearLeanVector * LeanDistance;
		MoveCameraByOffsetVector(OffsetVec, DeltaSeconds);
	}
}




void AHeroCharacter::MoveCameraByOffsetVector(const FVector2D& OffsetVec, float DeltaSeconds) const
{
	// Calculate a world space offset based on LeanVector
	const FVector Offset_WorldSpace = FVector{ OffsetVec.Y, OffsetVec.X, 0.f };

	// Find the origin of our camera offset node
	const FTransform CompTform = FollowCameraOffsetComp->GetComponentTransform();
	const FVector Origin_WorldSpace = CompTform.TransformPosition(FVector::ZeroVector);

	// Calc the goal location in world space by adding our offset to the origin in world space
	const FVector GoalLocation_WorldSpace = Origin_WorldSpace + Offset_WorldSpace;

	// Transform goal location from world space to cam offset space
	const FVector GoalLocation_LocalSpace = CompTform.InverseTransformPosition(GoalLocation_WorldSpace);

	// Move towards goal!
	const auto Current = FollowCameraOffsetComp->RelativeLocation;
	const auto Diff = GoalLocation_LocalSpace - Current;

	float Rate = bUseMouseAim && !bUseExperimentalMouseTracking
		? 1// Trying no cushion //LeanCushionRateMouse * DeltaSeconds
		: LeanCushionRateGamepad * DeltaSeconds;
	Rate = FMath::Clamp(Rate, 0.0f, 1.f);

	const FVector Change = Diff * Rate;

	FollowCameraOffsetComp->SetRelativeLocation(Current + Change);
}

FVector2D AHeroCharacter::TrackCameraWithAimMouse() const
{
	// Inputs
	const auto HeroCont = GetHeroController();
	const auto ViewportSize = GetGameViewportSize();
	FVector2D CursorLoc;

	if (!HeroCont || !HeroCont->GetMousePosition(OUT CursorLoc.X, OUT CursorLoc.Y) || ViewportSize.SizeSquared() <= 0)
	{
		return FVector2D::ZeroVector;
	}
	//UE_LOG(LogTemp, Warning, TEXT("Cursor: %s"), *CursorLoc.ToString());

	FVector2D LinearLeanVector = CalcLinearLeanVectorUnclipped(CursorLoc, ViewportSize);
	FVector2D ClippedLinearLeanVector;

	// Clip LeanVector
	
	if (ClippingModeMouse == 0)
	{
		// Circle
		ClippedLinearLeanVector = LinearLeanVector.SizeSquared() > 1
			? LinearLeanVector.GetSafeNormal()
			: LinearLeanVector;
	} 
	else if (ClippingModeMouse == 1)
	{
		// Square
		ClippedLinearLeanVector = LinearLeanVector.ClampAxes(-1.f, 1.f);
	}
	else if (ClippingModeMouse == 2)
	{
		// 16:9
		ClippedLinearLeanVector = LinearLeanVector.ClampAxes(-1.78f, 1.78f);
	}
	else
	{
		// Unbounded
		ClippedLinearLeanVector = LinearLeanVector; // unbounded
	}

	return ClippedLinearLeanVector;
}

FVector2D AHeroCharacter::TrackCameraWithAimGamepad() const
{
	FVector2D LinearLeanVector = FVector2D{ AxisFaceRight, AxisFaceUp };
	return LinearLeanVector;
}

void AHeroCharacter::ExperimentalMouseAimTracking(float DT)
{
	auto*const Hero = GetHeroController();
	if (!Hero) return;

	// Hide the cursor - NOTE the cursor still exists and works nicely, just cant see it!
	Hero->bShowMouseCursor = false;

	// Track change in mouse
	FVector2D MousePos;
	Hero->GetMousePosition(OUT MousePos.X, OUT MousePos.Y);
	FVector2D MouseDelta = MousePos - AimPos_ScreenSpace;
	AimPos_ScreenSpace = MousePos;

	//UE_LOG(LogTemp, Warning, TEXT("CursorPos: %s, Delta: %s"), *MousePos.ToString(), *MouseDelta.ToString());

	FVector WorldLocation;
	FVector WorldDirection;
	if (!UGameplayStatics::DeprojectScreenToWorld(Hero, MousePos, OUT WorldLocation, OUT WorldDirection))
	{
		UE_LOG(LogTemp, Error, TEXT("Couldn't project screen position into world"));
		return;
	}


	const FVector AnchorPos = WeaponAnchor->GetComponentLocation();
	AimPos_WorldSpace = FMath::LinePlaneIntersection(
		WorldLocation,
		WorldLocation + (WorldDirection * 5000),
		AnchorPos,
		FVector(0, 0, 1));

	AimPosComp->SetWorldLocation(AimPos_WorldSpace);

	const FVector AimVec = AimPos_WorldSpace - AnchorPos;

	// Face the player at this
	Controller->SetControlRotation((AimVec*-1).Rotation()); // reverse aim vec just to help see setcontrolrotation works here



	const auto ViewportSize = GetGameViewportSize();
	FVector2D LinearLeanVector = CalcLinearLeanVectorUnclipped(AimPos_ScreenSpace, ViewportSize);


	const auto OffsetVec = LinearLeanVector * LeanDistance;
	MoveCameraByOffsetVector(OffsetVec, DT);

	/*
	
	
	
	// Create a custom camera that contains a "frame offset pointer" pos

	// Get mouse input deltas (dx dy) and move the frame offset pointer accordingly

	// Project offset pointer into the character's aim plane to get world position

	//

	AimPos_World = GetMouseDelta.ProjectToAimPlane()

		AimPos_Screen = AimPos_World.ProjectToScreen()
		AimVec_Screen = AimPos_Screen - Center_Screen
		NormalizedAimVec_Screen = AimVec_Screen / (Viewport.Y / 2)

	*/
//	FVector2D{ AimVec.X, AimVec.Y };
}





FVector2D AHeroCharacter::CalcLinearLeanVectorUnclipped(const FVector2D& CursorLoc, const FVector2D& ViewportSize)
{
	const auto Mid = ViewportSize / 2.f;

	// Define a circle that touches the top and bottom of the screen
	const auto HalfSideLen = ViewportSize.Y / 2.f;

	// Create a vector from the middle to the cursor that has a length ratio relative to the radius
	const auto CursorVecFromMid = CursorLoc - Mid;
	const auto CursorVecRelative = CursorVecFromMid / HalfSideLen;
	
	// Flip Y
	return CursorVecRelative * FVector2D{ 1, -1 };
}

FVector2D AHeroCharacter::GetGameViewportSize()
{
	FVector2D Result{};

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(OUT Result);
	}

	return Result;
}

void AHeroCharacter::ApplyDamage(uint32 InstigatorHeroControllerId, float Damage, FVector Location)
{
	//This must only run on a dedicated server or listen server

	if (!HasAuthority()) return;

	auto bHitArmour = false;

	if (Armour > 0)
	{
		bHitArmour = true;

		if (Damage > Armour)
		{
			const float DamageToHealth = Damage - Armour;
			Armour = 0;
			Health -= DamageToHealth;
		}
		else
		{
			Armour -= Damage;
		}
	}
	else
	{
		Health -= Damage;
	}

	
	FString S = FString::Printf(TEXT("%fhp"), Health);
	//LogMsgWithRole(S);


	// Report hit to controller
	auto HC = GetHeroController();
	if (HC)
	{
		FMRHitResult Hit{};
		Hit.ReceiverControllerId = HC->GetUniqueID();
		Hit.AttackerControllerId = InstigatorHeroControllerId;
		Hit.HealthRemaining = (int)Health;
		Hit.DamageTaken = (int)Damage;
		Hit.bHitArmour = bHitArmour;
		Hit.HitLocation = Location;
		//Hit.HitDirection

		HC->DamageTaken(Hit);
	}
	
}

bool AHeroCharacter::TryGiveHealth(float Hp)
{
	//LogMsgWithRole("TryGiveHealth");
	if (!HasAuthority()) return false;

	if (Health == MaxHealth) return false;
	
	Health = FMath::Min(Health + Hp, MaxHealth);
	return true;
}

bool AHeroCharacter::TryGiveAmmo()
{
	//LogMsgWithRole("AHeroCharacter::TryGiveAmmo");
	if (!HasAuthority()) return false;

	if (CurrentWeapon != nullptr)
	{
		return CurrentWeapon->TryGiveAmmo();
	}

	return false;
}

bool AHeroCharacter::TryGiveArmour(float Delta)
{
	//LogMsgWithRole("TryGiveArmour");
	if (!HasAuthority()) return false;
	if (Armour == MaxArmour) return false;

	Armour = FMath::Min(Armour + Delta, MaxArmour);
	return true;
}

bool AHeroCharacter::TryGiveWeapon(const TSubclassOf<AWeapon>& Class)
{
	//LogMsgWithRole("AHeroCharacter::TryGiveWeapon");

	if (Class == nullptr) return false;
	if (!HasAuthority()) return false;

	
	if (CurrentWeapon)
	{
		// If we already have the gun, treat it as an ammo pickup!
		if (CurrentWeapon->IsA(Class))
		{
			return TryGiveAmmo();
		}

		// Otherwise, destroy our current weapon to make space for the new one!
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}


	ServerRPC_SpawnWeapon(Class);

	return true;
}

void AHeroCharacter::Input_Interact()
{
	//LogMsgWithRole("AHeroCharacter::Input_Interact()");
	ServerRPC_TryInteract();
}

void AHeroCharacter::ServerRPC_TryInteract_Implementation()
{
	//LogMsgWithRole("AHeroCharacter::ServerRPC_TryInteract_Implementation()");

	auto* const Pickup = ScanForInteractable<AWeaponPickupBase>();
	if (Pickup && Pickup->CanInteract())
	{
		//LogMsgWithRole("AHeroCharacter::ServerRPC_TryInteract_Implementation() : Found");
		Pickup->TryInteract(this);
	}
}

bool AHeroCharacter::ServerRPC_TryInteract_Validate()
{
	return true;
}

template <class T>
T* AHeroCharacter::ScanForInteractable()
{
	FHitResult Hit = GetFirstPhysicsBodyInReach();
	return Cast<T>(Hit.GetActor());
}

FHitResult AHeroCharacter::GetFirstPhysicsBodyInReach() const
{
	//LogMsgWithRole("AHeroCharacter::GetFirstPhysicsBodyInReach()");

	FVector traceStart, traceEnd;
	GetReachLine(OUT traceStart, OUT traceEnd);

	//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor{ 255,0,0 }, false, -1., 0, 5.f);

	// Raycast along line to find intersecting physics object
	FHitResult hitResult;
	bool isHit = GetWorld()->LineTraceSingleByObjectType(
		OUT hitResult,
		traceStart,
		traceEnd,
		FCollisionObjectQueryParams{ ECollisionChannel::ECC_WorldDynamic },//TODO Make a custom channel for interactables!
		FCollisionQueryParams{ FName(""), false, GetOwner() }
	);

	return hitResult;
}
void AHeroCharacter::GetReachLine(OUT FVector& outStart, OUT FVector& outEnd) const
{
	outStart = GetActorLocation();
	
	outStart.Z -= 60; // check about ankle height
	outEnd = outStart + GetActorRotation().Vector() * InteractableSearchDistance;
}

void AHeroCharacter::LogMsgWithRole(FString message) const
{
	FString m = GetRoleText() + ": " + message;
	UE_LOG(LogTemp, Warning, TEXT("%s"), *m);
}
FString AHeroCharacter::GetEnumText(ENetRole role) const
{
	switch (role) {
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "Sim";
	case ROLE_AutonomousProxy:
		return "Auto";
	case ROLE_Authority:
		return "Auth";
	case ROLE_MAX:
		return "MAX (error?)";
	default:
		return "ERROR";
	}
}
FString AHeroCharacter::GetRoleText() const
{
	auto Local = Role;
	auto Remote = GetRemoteRole();


	//if (Remote == ROLE_SimulatedProxy) //&& Local == ROLE_Authority
	//	return "ListenServer";

	//if (Local == ROLE_Authority)
	//	return "Server";

	//if (Local == ROLE_AutonomousProxy) // && Remote == ROLE_Authority
	//	return "OwningClient";

	//if (Local == ROLE_SimulatedProxy) // && Remote == ROLE_Authority
	//	return "SimClient";

	return GetEnumText(Role) + " " + GetEnumText(GetRemoteRole()) + " Ded:" + (IsRunningDedicatedServer() ? "True" : "False");

}
