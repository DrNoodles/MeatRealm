#include "HeroCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/Public/DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "UnrealNetwork.h"
#include "HeroState.h"
#include "HeroController.h"
#include "WeaponPickupBase.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon.h"


/// Lifecycle

AHeroCharacter::AHeroCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// Replication
	bAlwaysRelevant = true;

	JumpMaxCount = 0;

	// Configure character movement
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
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

	HolsteredweaponAnchor = CreateDefaultSubobject<UArrowComponent>(TEXT("HolsteredweaponAnchor"));
	HolsteredweaponAnchor->SetupAttachment(RootComponent);

	// Create TEMP aim pos comp to help visualise aiming target
	AimPosComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AimPosComp"));
	AimPosComp->SetupAttachment(RootComponent);
}

void AHeroCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ROLE_Authority == Role)
	{
		CurrentWeaponSlot = EWeaponSlots::Undefined;
		if (Slot1)
		{
			Slot1->Destroy();
			Slot1 = nullptr;
		}
		if (Slot2)
		{
			Slot2->Destroy();
			Slot2 = nullptr;
		}
	}
}

void AHeroCharacter::Restart()
{
	Super::Restart();
	LogMsgWithRole("AHeroCharacter::Restart()");

	Health = MaxHealth;
	Armour = 0.f;

	// Randomly select a weapon
	if (DefaultWeaponClass.Num() > 0)
	{
		if (HasAuthority())
		{
			const auto Choice = FMath::RandRange(0, DefaultWeaponClass.Num() - 1);

			const auto Weapon = AuthSpawnAndAttachWeapon(DefaultWeaponClass[Choice]);
			const auto Slot = FindGoodSlot();
			AssignWeaponToSlot(Weapon, Slot);
			EquipWeapon(Slot);
		}
	}
}

void AHeroCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHeroCharacter, CurrentWeaponSlot);
	DOREPLIFETIME(AHeroCharacter, Slot1);
	DOREPLIFETIME(AHeroCharacter, Slot2);
	DOREPLIFETIME(AHeroCharacter, Health);
	DOREPLIFETIME(AHeroCharacter, Armour);
	DOREPLIFETIME(AHeroCharacter, TeamTint);
	DOREPLIFETIME(AHeroCharacter, bIsAdsing);
}

void AHeroCharacter::Tick(float DeltaSeconds)
{
	// No need for server. We're only doing input processing and client effects here.
	if (HasAuthority()) return;


	// Local Client only below

	const auto HeroCont = GetHeroController();
	if (HeroCont == nullptr) return;



	// Handle Input (move and look)

	const auto deadzoneSquared = 0.25f * 0.25f;


	// Move character
	const auto moveVec = FVector{ AxisMoveUp, AxisMoveRight, 0 };

	// Debuf move speed if backpeddling
	auto CurrentLookVec = GetActorRotation().Vector();
	bool bBackpedaling = IsBackpedaling(moveVec.GetSafeNormal(), CurrentLookVec, BackpedalThresholdAngle);

	// TODO This needs to be server side or it's hackable!
	auto MoveScalar = bBackpedaling ? BackpedalSpeedMultiplier : 1.0f; 

	if (moveVec.SizeSquared() >= deadzoneSquared)
	{
		AddMovementInput(FVector{ MoveScalar, 0.f, 0.f }, moveVec.X);
		AddMovementInput(FVector{ 0.f, MoveScalar, 0.f }, moveVec.Y);
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
				FVector(0, 0, 1));

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
	
	float PickupDelay;
	if (Pickup && Pickup->CanInteract(this, OUT PickupDelay))
	{
		
		LogMsgWithRole(FString::Printf(TEXT("Can Interact with delay! %f "), PickupDelay));
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



	// Draw ADS line
	if (bIsAdsing) 
	{
		//DrawAdsLine(AdsLineColor, AdsLineLength);
	}
}




// Ads mode

void AHeroCharacter::SimulateAdsMode(bool IsAdsing)
{
	bIsAdsing = IsAdsing;

	float MoveSpeed = WalkSpeed;

	if (IsAdsing)
	{
		const auto Weapon = GetCurrentWeapon();
		if (Weapon)
		{
			MoveSpeed = WalkSpeed * Weapon->AdsMovementScale;
		}
	}

	GetCharacterMovement()->MaxWalkSpeed = MoveSpeed;
}
//
//void AHeroCharacter::DrawAdsLine(const FColor& Color, float LineLength) const
//{
//	const FVector Start = WeaponAnchor->GetComponentLocation();
//	FVector End = Start + WeaponAnchor->GetComponentRotation().Vector() * LineLength;
//
//	// Trace line to first hit for end
//	FHitResult HitResult;
//	const bool bIsHit = GetWorld()->LineTraceSingleByChannel(
//		OUT HitResult,
//		Start,
//		End,
//		ECC_Visibility,
//		FCollisionQueryParams{ FName(""), false, this }
//	);
//
//	if (bIsHit) End = HitResult.ImpactPoint;
//
//	DrawDebugLine(GetWorld(), Start, End, Color, false, -1., 0, 2.f);
//}

void AHeroCharacter::ServerRPC_AdsReleased_Implementation()
{
	SimulateAdsMode(false);
}

bool AHeroCharacter::ServerRPC_AdsReleased_Validate()
{
	return true;
}

void AHeroCharacter::ServerRPC_AdsPressed_Implementation()
{
	SimulateAdsMode(true);
}

bool AHeroCharacter::ServerRPC_AdsPressed_Validate()
{
	return true;
}

void AHeroCharacter::Input_FirePressed() const
{
	if (GetCurrentWeapon()) GetCurrentWeapon()->Input_PullTrigger();
}

void AHeroCharacter::Input_FireReleased() const
{
	if (GetCurrentWeapon()) GetCurrentWeapon()->Input_ReleaseTrigger();
}

void AHeroCharacter::Input_AdsPressed()
{
	if (GetCurrentWeapon()) GetCurrentWeapon()->Input_AdsPressed();
	SimulateAdsMode(true);
	ServerRPC_AdsPressed();
}

void AHeroCharacter::Input_AdsReleased()
{
	if (GetCurrentWeapon()) GetCurrentWeapon()->Input_AdsReleased();
	SimulateAdsMode(false);
	ServerRPC_AdsReleased();
}

void AHeroCharacter::Input_Reload() const
{
	if (GetCurrentWeapon()) GetCurrentWeapon()->Input_Reload();
}




// Weapon spawning
AWeapon* AHeroCharacter::GetWeapon(EWeaponSlots Slot) const
{
	switch (Slot) 
	{ 
	case EWeaponSlots::Primary: return Slot1;
	case EWeaponSlots::Secondary: return Slot2;
	
	case EWeaponSlots::Undefined:
	default:
		return nullptr; 
	}
}
void AHeroCharacter::SetWeapon(AWeapon* Weapon, EWeaponSlots Slot)
{
	// This does not free up resources by design! If needed, first get the weapon before overwriting it
	if (Slot == EWeaponSlots::Primary) Slot1 = Weapon;
	if (Slot == EWeaponSlots::Secondary) Slot2 = Weapon;
}

AWeapon* AHeroCharacter::GetCurrentWeapon() const
{
	return GetWeapon(CurrentWeaponSlot);
}

AWeapon* AHeroCharacter::AuthSpawnAndAttachWeapon(TSubclassOf<AWeapon> weaponClass)
{
	//LogMsgWithRole("AHeroCharacter::ServerRPC_SpawnWeapon");
	check(HasAuthority())
	if (!GetWorld()) return nullptr;


	// Spawn the weapon at the anchor
	auto* Weapon = GetWorld()->SpawnActorDeferred<AWeapon>(
		weaponClass,
		WeaponAnchor->GetComponentTransform(),
		this,
		this,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	
	if (Weapon == nullptr) { return nullptr; }


	// Configure it
	Weapon->AttachToComponent(WeaponAnchor, FAttachmentTransformRules{ EAttachmentRule::KeepWorld, true });
	Weapon->SetHeroControllerId(GetHeroController()->PlayerState->PlayerId);


	// Finish him!
	UGameplayStatics::FinishSpawningActor(
		Weapon,
		WeaponAnchor->GetComponentTransform());

	return Weapon;
}

EWeaponSlots AHeroCharacter::FindGoodSlot() const
{
	// Find an empty slot, if none exists, 
	if (!Slot1) return EWeaponSlots::Primary;
	if (!Slot2) return EWeaponSlots::Secondary;
	return CurrentWeaponSlot;
}
void AHeroCharacter::AssignWeaponToSlot(AWeapon* Weapon, EWeaponSlots Slot)
{
	auto Removed = GetWeapon(Slot);
	SetWeapon(Weapon, Slot);

	// Cleanup previous weapon // TODO Drop this on ground
	if (Removed) Removed->Destroy();
}

void AHeroCharacter::EquipWeapon(const EWeaponSlots Slot)
{
	if (CurrentWeaponSlot == Slot) return;

	// If desired slot is empty, do nothing.
	auto NewWeapon = GetWeapon(Slot);
	if (!NewWeapon) return;

	const auto OldSlot = CurrentWeaponSlot;
	CurrentWeaponSlot = Slot;


	// TODO Some management code here to delay for the duration of holster/draw and cancel if certain things happen

	float HolsterDuration = 0;

	// Hide old weapon
	auto OldWeapon = GetWeapon(OldSlot);
	if (OldWeapon)
	{
		//OldWeapon->SetActorHiddenInGame(true);
		OldWeapon->QueueHolster();
		OldWeapon->AttachToComponent(HolsteredweaponAnchor, 
			FAttachmentTransformRules{ EAttachmentRule::SnapToTarget, true });
		HolsterDuration = OldWeapon->HolsterDuration;
	}


	// Show new weapon
	if (NewWeapon)
	{
		//NewWeapon->SetActorHiddenInGame(false);
		NewWeapon->Draw();
		NewWeapon->AttachToComponent(WeaponAnchor,
			FAttachmentTransformRules{ EAttachmentRule::SnapToTarget, true });
	}


	// TODO Swap attatchment points (eg, new gun in hands, old gun on back)
}


// Camera tracks aim

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




// Affect the character

void AHeroCharacter::AuthApplyDamage(uint32 InstigatorHeroControllerId, float Damage, FVector Location)
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
		Hit.ReceiverControllerId = HC->PlayerState->PlayerId;
		Hit.AttackerControllerId = InstigatorHeroControllerId;
		Hit.HealthRemaining = (int)Health;
		Hit.DamageTaken = (int)Damage;
		Hit.bHitArmour = bHitArmour;
		Hit.HitLocation = Location;
		//Hit.HitDirection

		HC->TakeDamage2(Hit);
	}
	
}

bool AHeroCharacter::AuthTryGiveHealth(float Hp)
{
	//LogMsgWithRole("TryGiveHealth");
	if (!HasAuthority()) return false;

	if (Health == MaxHealth) return false;
	
	Health = FMath::Min(Health + Hp, MaxHealth);
	return true;
}

bool AHeroCharacter::AuthTryGiveAmmo()
{
	//LogMsgWithRole("AHeroCharacter::TryGiveAmmo");
	if (!HasAuthority()) return false;

	if (GetCurrentWeapon() != nullptr)
	{
		return GetCurrentWeapon()->TryGiveAmmo();
	}

	return false;
}

bool AHeroCharacter::AuthTryGiveArmour(float Delta)
{
	//LogMsgWithRole("TryGiveArmour");
	if (!HasAuthority()) return false;
	if (Armour == MaxArmour) return false;

	Armour = FMath::Min(Armour + Delta, MaxArmour);
	return true;
}

bool AHeroCharacter::AuthTryGiveWeapon(const TSubclassOf<AWeapon>& Class)
{
	check(HasAuthority());
	check(Class != nullptr);

	//LogMsgWithRole("AHeroCharacter::TryGiveWeapon");

	if (GetCurrentWeapon() && GetCurrentWeapon()->IsA(Class))
	{
		// If we already have the gun equipped, treat it as an ammo pickup!
		return AuthTryGiveAmmo();
	}

	const auto Weapon = AuthSpawnAndAttachWeapon(Class);
	const auto Slot = FindGoodSlot();
	AssignWeaponToSlot(Weapon, Slot);
	EquipWeapon(Slot);

	return true;
}

float AHeroCharacter::GetGiveWeaponDelay()
{
	const bool bIdealSlotAlreadyContainsWeapon = GetWeapon(FindGoodSlot()) != nullptr;
	if (bIdealSlotAlreadyContainsWeapon)
	{
		return 2.; // delay pickup
	}

	return 0.f;
}


// Interacting

void AHeroCharacter::Input_Interact()
{
	//LogMsgWithRole("AHeroCharacter::Input_Interact()");
	ServerRPC_TryInteract();
}

void AHeroCharacter::Input_PrimaryWeapon()
{
	ServerRPC_EquipPrimaryWeapon();
}
void AHeroCharacter::Input_SecondaryWeapon()
{
	ServerRPC_EquipSecondaryWeapon();
}
void AHeroCharacter::Input_ToggleWeapon()
{
	ServerRPC_ToggleWeapon();
}

void AHeroCharacter::ServerRPC_TryInteract_Implementation()
{
	//LogMsgWithRole("AHeroCharacter::ServerRPC_TryInteract_Implementation()");

	auto* const Pickup = ScanForInteractable<AWeaponPickupBase>();

	float PickupDelay;

	// TODO Write delayed interaction here if PickupDelay is > SMALL_NUMBER

	if (Pickup && Pickup->CanInteract(this, OUT PickupDelay))
	{
		//LogMsgWithRole("AHeroCharacter::ServerRPC_TryInteract_Implementation() : Found");
		Pickup->AuthTryInteract(this);
	}
}

bool AHeroCharacter::ServerRPC_TryInteract_Validate()
{
	return true;
}


void AHeroCharacter::ServerRPC_EquipPrimaryWeapon_Implementation()
{
	EquipWeapon(EWeaponSlots::Primary);
}

bool AHeroCharacter::ServerRPC_EquipPrimaryWeapon_Validate()
{
	return true;
}

void AHeroCharacter::ServerRPC_EquipSecondaryWeapon_Implementation()
{
	EquipWeapon(EWeaponSlots::Secondary);
}

bool AHeroCharacter::ServerRPC_EquipSecondaryWeapon_Validate()
{
	return true;
}


void AHeroCharacter::ServerRPC_ToggleWeapon_Implementation()
{
	switch (CurrentWeaponSlot)
	{
	case EWeaponSlots::Primary:
		EquipWeapon(EWeaponSlots::Secondary);
		break;

	case EWeaponSlots::Undefined:
	case EWeaponSlots::Secondary:
	default:
		EquipWeapon(EWeaponSlots::Primary);
	}
}

bool AHeroCharacter::ServerRPC_ToggleWeapon_Validate()
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
	
	bool bDrawDebug = false;
	if (bDrawDebug) DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor{ 255,0,0 }, false, -1., 0, 3.f);

	// Raycast along line to find intersecting physics object
	FHitResult hitResult;
	bool isHit = GetWorld()->LineTraceSingleByObjectType( // TODO Convert to 
		OUT hitResult,
		traceStart,
		traceEnd,
		FCollisionObjectQueryParams{ ECollisionChannel::ECC_GameTraceChannel2 },
		FCollisionQueryParams{ FName(""), false, GetOwner() }
	);

	if (isHit && bDrawDebug)
	{
		DrawDebugLine(GetWorld(), traceStart, hitResult.ImpactPoint, FColor{ 0,0,255 }, false, -1., 0, 5.f);
	}

	return hitResult;
}
void AHeroCharacter::GetReachLine(OUT FVector& outStart, OUT FVector& outEnd) const
{
	outStart = GetActorLocation();
	
	outStart.Z -= 60; // check about ankle height
	outEnd = outStart + GetActorRotation().Vector() * InteractableSearchDistance;
}




// Other

void AHeroCharacter::OnRep_TintChanged() const
{
	/*LogMsgWithRole(
		FString::Printf(TEXT("AHeroCharacter::OnRep_TintChanged() %s"), *TeamTint.ToString())
	);*/
	OnPlayerTintChanged.Broadcast();
}

AHeroState* AHeroCharacter::GetHeroState() const
{
	return GetPlayerState<AHeroState>();
}

AHeroController* AHeroCharacter::GetHeroController() const
{
	return GetController<AHeroController>();
}

bool AHeroCharacter::IsBackpedaling(const FVector& MoveDir, const FVector& AimDir, int BackpedalAngle)
{
	const bool bBackpedaling = FVector::DotProduct(MoveDir, AimDir) < FMath::Cos(FMath::DegreesToRadians(BackpedalAngle));

	// Debug
	if (true && bBackpedaling)
	{
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(MoveDir, AimDir)));
		UE_LOG(LogTemp, Warning, TEXT("Backpedaling! %f"), Angle);
	}
		

	return bBackpedaling;
}


// Debug logging

void AHeroCharacter::LogMsgWithRole(FString message) const
{
	FString m = GetRoleText() + ": " + message;
	UE_LOG(LogTemp, Warning, TEXT("%s"), *m);
}
FString AHeroCharacter::GetRoleText() const
{
	return GetEnumText(Role) + " " + GetEnumText(GetRemoteRole());
}
FString AHeroCharacter::GetEnumText(ENetRole role)
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
