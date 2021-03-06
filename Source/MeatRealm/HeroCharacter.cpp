#include "HeroCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
#include "MeatyCharacterMovementComponent.h"
#include "WeaponPickupBase.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Interfaces/Equippable.h"

/// Lifecycle

AHeroCharacter::AHeroCharacter(const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer.SetDefaultSubobjectClass<UMeatyCharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName))
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
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->MaxAcceleration = 3000;

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

	LastRunEnded = FDateTime::Now();
}

void AHeroCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ROLE_Authority == Role)
	{
		LastInventorySlot = EInventorySlots::Undefined;
		CurrentInventorySlot = EInventorySlots::Undefined;
		if (PrimaryWeaponSlot)
		{
			PrimaryWeaponSlot->Destroy();
			PrimaryWeaponSlot = nullptr;
		}
		if (SecondaryWeaponSlot)
		{
			SecondaryWeaponSlot->Destroy();
			SecondaryWeaponSlot = nullptr;
		}

		for (auto* HP: HealthSlot)
		{
			HP->Destroy();
		}

		for (auto* AP : ArmourSlot)
		{
			AP->Destroy();
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
			const auto WeaponClass = DefaultWeaponClass[Choice];
			auto Config = FWeaponConfig{};

			GiveWeaponToPlayer(WeaponClass, Config);
		}
	}
}

void AHeroCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Everyone!
	DOREPLIFETIME(AHeroCharacter, TeamTint);

	// Everyone except local owner: flag change is locally instigated
	DOREPLIFETIME_CONDITION(AHeroCharacter, bIsRunning, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AHeroCharacter, bIsTargeting, COND_SkipOwner);

	// Just the owner
	DOREPLIFETIME_CONDITION(AHeroCharacter, Armour, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AHeroCharacter, LastInventorySlot, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AHeroCharacter, CurrentInventorySlot, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AHeroCharacter, PrimaryWeaponSlot, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AHeroCharacter, SecondaryWeaponSlot, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AHeroCharacter, HealthSlot, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AHeroCharacter, ArmourSlot, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AHeroCharacter, Health, COND_OwnerOnly);
}

void AHeroCharacter::ScanForWeaponPickups(float DeltaSeconds)
{
	auto* const Pickup = ScanForInteractable<APickupBase>();

	float PickupDelay;
	if (Pickup && Pickup->CanInteract(this, OUT PickupDelay))
	{
		//LogMsgWithRole(FString::Printf(TEXT("Can Interact with delay! %f "), PickupDelay));
		auto World = GetWorld();

		auto* InputSettings = UInputSettings::GetInputSettings();
		TArray<FInputActionKeyMapping> Mappings;
		InputSettings->GetActionMappingByName("Interact", OUT Mappings);

		FString ActionText = "Undefined";
		for (FInputActionKeyMapping Mapping : Mappings)
		{
			bool canUseKeyboardKey = bUseMouseAim && !Mapping.Key.IsGamepadKey();
			bool canUseGamepadKey = !bUseMouseAim && Mapping.Key.IsGamepadKey();

			if (canUseKeyboardKey)
			{
				ActionText = Mapping.Key.GetDisplayName().ToString();
				break;
			}

			// Hacky! Make the gamepad text not bloody horrible
			if (canUseGamepadKey)
			{
				ActionText = Mapping.Key.ToString();
				const FString S = Mapping.Key.ToString();

				if (S == "Gamepad_FaceButton_Bottom") { ActionText = "A"; break;}
				if (S == "Gamepad_FaceButton_Right") { ActionText = "B"; break;}
				if (S == "Gamepad_FaceButton_Left") { ActionText = "X"; break;}
				if (S == "Gamepad_FaceButton_Top") { ActionText = "Y"; break;}
				if (S == "Gamepad_DPad_Up") { ActionText = "Up"; break;}
				if (S == "Gamepad_DPad_Down") { ActionText = "Down"; break;}
				if (S == "Gamepad_DPad_Right") { ActionText = "Right"; break;}
				if (S == "Gamepad_DPad_Left") { ActionText = "Left"; break;}
				if (S == "Gamepad_LeftShoulder") { ActionText = "LB"; break;}
				if (S == "Gamepad_RightShoulder") { ActionText = "RB"; break;}
				if (S == "Gamepad_LeftTrigger") { ActionText = "LT"; break;}
				if (S == "Gamepad_RightTrigger") { ActionText = "RT"; break;}
				/*
				static const FKey Gamepad_LeftThumbstick;
				static const FKey Gamepad_RightThumbstick;
				static const FKey Gamepad_Special_Left;
				static const FKey Gamepad_Special_Left_X;
				static const FKey Gamepad_Special_Left_Y;
				static const FKey Gamepad_Special_Right;

				*/
			}
			
		}
		if (World)
		{
			auto Str = FString::Printf(TEXT("Grab %s (%s)"), *Pickup->GetPickupName(), *ActionText);
			const auto YOffset = -5.f * Str.Len();

			DrawDebugString(World, FVector{ 50, YOffset, 100 },	Str, Pickup, FColor::White, DeltaSeconds * 0.5);
		}

		//LogMsgWithRole("Can Interact! ");
	}
}

void AHeroCharacter::Tick(float DeltaSeconds)
{
	// No need for server. We're only doing input processing and client effects here.
	if (HasAuthority()) return;
	if (GetHeroController() == nullptr) return;

	if (bDrawMovementInput)
	{
		auto V = FVector{ AxisMoveUp, AxisMoveRight, 0 } * 100;
		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + V, 3, FColor::Blue, false, -1, 0, 2.f);
	}
	if (bDrawMovementVector)
	{
		auto V = GetVelocity();
		DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + V, 3, FColor::Green, false, -1, 0, 2.f);
	}

	// Local Client only below
	if (IsRunning())// && GetVelocity().Size() > WalkSpeed*.7)
	{
		TickRunning(DeltaSeconds);
	}
	else
	{
		TickWalking(DeltaSeconds);
	}

	ScanForWeaponPickups(DeltaSeconds);


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


	// Draw drawing weapon debug text

	if (GetCurrentWeapon() && GetCurrentWeapon()->IsEquipping())
	{
		auto str = FString::Printf(TEXT("Equipping %s"), *GetCurrentWeapon()->GetWeaponName());
		const auto YOffset = -5.f * str.Len();
		DrawDebugString(GetWorld(), FVector{ 70, YOffset, 50 }, str, this, FColor::White, DeltaSeconds * 0.7);
	}
}

void AHeroCharacter::TickWalking(float DT)
{
	const auto deadzoneSquared = Deadzone * Deadzone;
	const auto HeroCont = GetHeroController();

	// Move character
	auto MoveVec = FVector{ AxisMoveUp, AxisMoveRight, 0 }.GetClampedToMaxSize(1);


	// Debuf move speed if backpeddling
	auto CurrentLookVec = GetActorRotation().Vector();
	bool bBackpedaling = IsBackpedaling(MoveVec.GetSafeNormal(), CurrentLookVec, BackpedalThresholdAngle);

	// TODO This needs to be server side or it's hackable!
	auto MoveScalar = bBackpedaling ? BackpedalSpeedMultiplier : 1.0f;


	if (MoveVec.SizeSquared() >= deadzoneSquared)
	{
		AddMovementInput(FVector{ MoveScalar, 0.f, 0.f }, MoveVec.X);
		AddMovementInput(FVector{ 0.f, MoveScalar, 0.f }, MoveVec.Y);
	}


	// Calculate Look Vector for mouse or gamepad
	FVector LookVec;

	if (bUseMouseAim)
	{
		FVector WorldLocation, WorldDirection;
		const auto Success = HeroCont->DeprojectMousePositionToWorld(OUT WorldLocation, OUT WorldDirection);
		if (Success)
		{
			const FVector AimStart = GetAimTransform().GetLocation();

			const FVector CursorHit = FMath::LinePlaneIntersection(
				WorldLocation,
				WorldLocation + (WorldDirection * 5000),
				AimStart,
				FVector(0, 0, 1));


			// TODO FIXME This is a hacky fix to stop the character spazzing out when the cursor is close to the weapon.
			const FVector PawnLocationOnAimPlane = FVector{ GetActorLocation().X, GetActorLocation().Y, AimStart.Z };
			const float PawnDistToCursor = FVector::Dist(CursorHit, PawnLocationOnAimPlane);
			const float PawnDistToBarrel = FVector::Dist(AimStart, PawnLocationOnAimPlane);
			
			// If cursor is between our pawn and the barrel, aim from our location, not the gun's.
			if (PawnDistToCursor < PawnDistToBarrel * 2)
			{
				//LogMsgWithRole("Short aim");
				LookVec = CursorHit - PawnLocationOnAimPlane;
			}
			else
			{
				LookVec = CursorHit - AimStart;
			}

		}
	}
	else // Gamepad
	{
		LookVec = FVector{ AxisFaceUp, AxisFaceRight, 0 };
	}

	// Apply Look Vector - Aim character with look, if look is below deadzone then try use move vec
	if (LookVec.SizeSquared() >= deadzoneSquared)
	{
		Controller->SetControlRotation(LookVec.Rotation());
	}
	else if (MoveVec.SizeSquared() >= deadzoneSquared)
	{
		Controller->SetControlRotation(MoveVec.Rotation());
	}
}

void AHeroCharacter::TickRunning(float DT)
{
	if (bDrawMovementSpeed)
	{
		FString str = FString::Printf(TEXT("Running! "));
		str.AppendInt((int)GetVelocity().Size());
		//FString str = FString::Printf(TEXT("Running! %d"), (int)GetVelocity().Size());
		DrawDebugString(GetWorld(), FVector{ -50, -50, -50 }, str, this, FColor::White, DT * 0.7);
	}


	const auto DeadzoneSquared = Deadzone * Deadzone;
	const FVector InputVector = FVector{ AxisMoveUp, AxisMoveRight, 0 }.GetClampedToMaxSize(1);

	// If input is zero and velocity is zero. Stop running.
	if (InputVector.SizeSquared() < DeadzoneSquared)
	{
		if (GetVelocity().Size() < 150)
		{
			LogMsgWithRole("Stopped running due to no input or velocity");
			SetRunning(false);
		}

		return;
	}



	// Set Movement direction
	AddMovementInput({ 1,0,0 }, InputVector.X);
	AddMovementInput({ 0,1,0 }, InputVector.Y);



	if (GetCharacterMovement()->bOrientRotationToMovement) return;

	// Slowly turn towards the movement direction (purely cosmetic)
	const auto FacingVector = GetActorForwardVector();
	const float DegreesAwayFromMove = FMath::RadiansToDegrees(FMath::Acos(FacingVector | InputVector));
	if (DegreesAwayFromMove < 3)
	{
		// Just snap to it - otherwise it'll oscillate around the desired direction
		Controller->SetControlRotation(InputVector.Rotation());
	}
	else
	{
		float Diff = RunTurnRateMax - RunTurnRateBase;
		float Ratio = DegreesAwayFromMove / 180.f;
		float EffectiveRate = RunTurnRateBase + Diff*Ratio;


		// Rotate over time
		const FVector FacingTangent = FVector::CrossProduct(FacingVector, FVector{ 0,0,1 });
		const bool IsLeftTurn = FVector::DotProduct(FacingTangent, InputVector) > 0;
		const int Dir = IsLeftTurn ? -1 : 1;
		AddControllerYawInput(EffectiveRate * Dir * DT);
	}
}



// Input
void AHeroCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("RunToggle", IE_Pressed, this, &AHeroCharacter::OnRunToggle);
	PlayerInputComponent->BindAction("Run", IE_Pressed, this, &AHeroCharacter::OnStartRunning);
	PlayerInputComponent->BindAction("Run", IE_Released, this, &AHeroCharacter::OnStopRunning);
	PlayerInputComponent->BindAction("EquipHealth", IE_Pressed, this, &AHeroCharacter::OnEquipHealth);
	PlayerInputComponent->BindAction("EquipArmour", IE_Pressed, this, &AHeroCharacter::OnEquipArmour);
	PlayerInputComponent->BindAction("EquipSmartHeal", IE_Pressed, this, &AHeroCharacter::OnEquipSmartHeal);

}

void AHeroCharacter::UseItemPressed() const
{
	//LogMsgWithRole("AHeroCharacter::UseItemPressed");
	auto Item = GetCurrentItem();
	if (Item) Item->UsePressed();
}
void AHeroCharacter::UseItemReleased() const
{
	//LogMsgWithRole("AHeroCharacter::UseItemReleased");
	auto Item = GetCurrentItem();
	if (Item) Item->UseReleased();
}
void AHeroCharacter::UseItemCancelled() const
{
	//LogMsgWithRole("AHeroCharacter::UseItemCancelled");
	auto Item = GetCurrentItem();
	if (Item) Item->Cancel();
}

void AHeroCharacter::OnEquipSmartHeal()
{
	if (Role < ROLE_Authority)
	{
		ServerEquipSmartHeal();
	}
	
	EquipSmartHeal();
}
void AHeroCharacter::EquipSmartHeal()
{
	// Equip priority Armour, then Health, then nothing
	auto Item = GetCurrentItem();

	if (ArmourSlot.Num() > 0 && CanGiveArmour())
	{
		if (Item && Item->IsAutoUseOnEquip() && Item->GetInventoryCategory() == EInventoryCategory::Armour)
		{
			Item->UsePressed();
		}
		else
		{
			EquipSlot(EInventorySlots::Armour);
		}
	}
	else if (HealthSlot.Num() > 0 && CanGiveHealth())
	{
		if (Item && Item->IsAutoUseOnEquip() && Item->GetInventoryCategory() == EInventoryCategory::Health)
		{
			Item->UsePressed();
		}
		else
		{
			EquipSlot(EInventorySlots::Health);
		}
	}
}
void AHeroCharacter::ServerEquipSmartHeal_Implementation()
{
	EquipSmartHeal();
}
bool AHeroCharacter::ServerEquipSmartHeal_Validate()
{
	return true;
}

void AHeroCharacter::OnEquipHealth()
{
	EquipHealth();

	if (Role < ROLE_Authority)
	{
		ServerEquipHealth();
	}
}
void AHeroCharacter::EquipHealth()
{
	auto Item = GetCurrentItem();
	if (Item && Item->IsAutoUseOnEquip() && Item->GetInventoryCategory() == EInventoryCategory::Health)
	{
		Item->UsePressed();
	}
	else
	{
		EquipSlot(EInventorySlots::Health);
	}
}
void AHeroCharacter::ServerEquipHealth_Implementation()
{
	EquipHealth();
}
bool AHeroCharacter::ServerEquipHealth_Validate()
{
	return true;
}

void AHeroCharacter::OnEquipArmour()
{
	EquipArmour();

	if (Role < ROLE_Authority)
	{
		ServerEquipArmour();
	}
}
void AHeroCharacter::EquipArmour()
{
	auto Item = GetCurrentItem();
	if (Item && Item->IsAutoUseOnEquip() && Item->GetInventoryCategory() == EInventoryCategory::Armour)
	{
		Item->UsePressed();
	}
	else
	{
		EquipSlot(EInventorySlots::Armour);
	}
}
void AHeroCharacter::ServerEquipArmour_Implementation()
{
	EquipArmour();
}
bool AHeroCharacter::ServerEquipArmour_Validate()
{
	return true;
}

void AHeroCharacter::OnEquipPrimaryWeapon()
{
	if (Role < ROLE_Authority)
	{
		ServerEquipPrimaryWeapon();
	}

	EquipSlot(EInventorySlots::Primary);
}
void AHeroCharacter::ServerEquipPrimaryWeapon_Implementation()
{
	OnEquipPrimaryWeapon();
}
bool AHeroCharacter::ServerEquipPrimaryWeapon_Validate()
{
	return true;
}

void AHeroCharacter::OnEquipSecondaryWeapon()
{
	if (Role < ROLE_Authority)
	{
		ServerEquipSecondaryWeapon();
	}

	EquipSlot(EInventorySlots::Secondary);
}
void AHeroCharacter::ServerEquipSecondaryWeapon_Implementation()
{
	OnEquipSecondaryWeapon();
}
bool AHeroCharacter::ServerEquipSecondaryWeapon_Validate()
{
	return true;
}

void AHeroCharacter::OnToggleWeapon()
{
	if (Role < ROLE_Authority)
	{
		ServerToggleWeapon();
	}

	ToggleWeapon();
}
void AHeroCharacter::ToggleWeapon()
{
	EInventorySlots TargetSlot;

	if (HasAWeaponEquipped())
	{
		if (CurrentInventorySlot == EInventorySlots::Primary)
		{
			TargetSlot = EInventorySlots::Secondary;
		}
		else
		{
			TargetSlot = EInventorySlots::Primary;
		}
	}
	else // Not a weapon!
	{
		if (LastInventorySlot == EInventorySlots::Primary || LastInventorySlot == EInventorySlots::Secondary)
		{
			TargetSlot = LastInventorySlot;
		}
		else
		{
			TargetSlot = EInventorySlots::Primary; // Default to primary
		}
	}

	EquipSlot(TargetSlot);
}
void AHeroCharacter::ServerToggleWeapon_Implementation()
{
	ToggleWeapon();
}
bool AHeroCharacter::ServerToggleWeapon_Validate()
{
	return true;
}



void AHeroCharacter::OnRunToggle()
{
	if (IsRunning())
		OnStopRunning();
	else
		OnStartRunning();
}
void AHeroCharacter::OnStartRunning()
{
	auto* MyPC = Cast<AHeroController>(Controller);
	if (MyPC /*&& MyPC->IsGameInputAllowed()*/)
	{
		if (IsTargeting())
		{
			SetTargeting(false);
		}
		StopWeaponFire();
		SetRunning(true);
	}
}

void AHeroCharacter::OnStopRunning()
{
	auto* MyPC = Cast<AHeroController>(Controller);
	if (MyPC /*&& MyPC->IsGameInputAllowed()*/)
	{
		SetRunning(false);
	}
}

void AHeroCharacter::SetRunning(bool bNewIsRunning)
{
	if (bIsRunning == bNewIsRunning) return;
	bIsRunning = bNewIsRunning;
	
	RefreshWeaponAttachments();

	// Do nothing if we aren't running 
	if (bNewIsRunning)
	{
		// Config movement properties TODO Make these data driven (BP) and use Meaty char movement comp
		GetCharacterMovement()->MaxAcceleration = 1250;
		GetCharacterMovement()->BrakingFrictionFactor = 1;
		GetCharacterMovement()->BrakingDecelerationWalking = 250;
		
		if (bCancelReloadOnRun && GetCurrentWeapon()) GetCurrentWeapon()->CancelAnyReload();
	}
	else // Is Walking 
	{
		// Config movement properties TODO Make these data driven (BP) and use Meaty char movement comp
		GetCharacterMovement()->MaxAcceleration = 3000;
		GetCharacterMovement()->BrakingFrictionFactor = 2;
		GetCharacterMovement()->BrakingDecelerationWalking = 3000;

		LastRunEnded = FDateTime::Now();
	}

	

	if (Role < ROLE_Authority)
	{
		ServerSetRunning(bNewIsRunning);
	}
}

void AHeroCharacter::ServerSetRunning_Implementation(bool bNewWantsToRun)
{
	SetRunning(bNewWantsToRun);
}

bool AHeroCharacter::ServerSetRunning_Validate(bool bNewWantsToRun)
{
	return true;
}




// Ads mode
bool AHeroCharacter::IsTargeting() const
{
	return bIsTargeting;
}

void AHeroCharacter::SetTargeting(bool bNewTargeting)
{
	bIsTargeting = bNewTargeting;



	if (GetCurrentWeapon())
	{
		if (bNewTargeting)
		{
			const FTimespan TimeSinceRun = FDateTime::Now() - LastRunEnded;
			const FTimespan RemainingTime = FTimespan::FromSeconds(RunCooldown) - TimeSinceRun;
			
			
			if (RemainingTime > 0)
			{
				// Delay fire!
				auto DelayAds = [&]
				{
					if (bIsTargeting && GetCurrentWeapon())
					{
						GetCurrentWeapon()->Input_AdsPressed();
					}
				};

				GetWorld()->GetTimerManager().SetTimer(RunEndTimerHandle, DelayAds, RemainingTime.GetTotalSeconds(), false);
			}
			else
			{
				GetCurrentWeapon()->Input_AdsPressed();
			}
		}
		else
		{
			GetCurrentWeapon()->Input_AdsReleased();
		}
	}


	//if (TargetingSound)
	//{
	//	UGameplayStatics::SpawnSoundAttached(TargetingSound, GetRootComponent());
	//}


	if (Role < ROLE_Authority)
	{
		ServerSetTargeting(bNewTargeting);
	}
}

bool AHeroCharacter::ServerSetTargeting_Validate(bool bNewTargeting)
{
	return true;
}

void AHeroCharacter::ServerSetTargeting_Implementation(bool bNewTargeting)
{
	SetTargeting(bNewTargeting);
}

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

void AHeroCharacter::Input_PrimaryPressed()
{
	auto* MyPC = Cast<AHeroController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (HasAnItemEquipped())
		{
			UseItemPressed();
		}
		else if (HasAWeaponEquipped())
		{
			if (bIsRunning || IsRunning())
			{
				SetRunning(false);
			}
			StartWeaponFire();
		}
	}
}

void AHeroCharacter::Input_PrimaryReleased()
{
	if (HasAnItemEquipped())
	{
		UseItemReleased();
	}
	else if (HasAWeaponEquipped())
	{
		StopWeaponFire();
	}
}

void AHeroCharacter::Input_SecondaryPressed()
{
	auto* MyPC = Cast<AHeroController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (HasAnItemEquipped())
		{
			UseItemCancelled();
		}
		else if (HasAWeaponEquipped())
		{
			if (bIsRunning || IsRunning())
			{
				SetRunning(false);
			}
			SetTargeting(true);
		}
	}
}

void AHeroCharacter::Input_SecondaryReleased()
{
	if (HasAWeaponEquipped())
	{
		SetTargeting(false);
	}
}

void AHeroCharacter::Input_Reload() const
{
	auto* MyPC = Cast<AHeroController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsRunning() && bCancelReloadOnRun) return; // don't start a reload if not allowed to reload while running

		if (GetCurrentWeapon())
		{
			GetCurrentWeapon()->Input_Reload();
		}
	}
}


// TODO Make this handle both client/server requests to fire - like ads pressed etc.
void AHeroCharacter::StartWeaponFire()
{
	//LogMsgWithRole("AHeroCharacter::StartWeaponFire");
	if (!bWantsToFire)
	{
		bWantsToFire = true;


		const FTimespan TimeSinceRun = FDateTime::Now() - LastRunEnded;
		const FTimespan RemainingTime = FTimespan::FromSeconds(RunCooldown) - TimeSinceRun;
		if (RemainingTime > 0)
		{
			// Delay fire!
			auto DelayFire = [&]
			{
				if (bWantsToFire && GetCurrentWeapon())
				{
					GetCurrentWeapon()->Input_PullTrigger();
				}
			};

			GetWorld()->GetTimerManager().SetTimer(RunEndTimerHandle, DelayFire, RemainingTime.GetTotalSeconds(), false);
			return;
		}

		// Fire now!
		else if (GetCurrentWeapon())
		{
			GetCurrentWeapon()->Input_PullTrigger();
		}
	}
}

void AHeroCharacter::StopWeaponFire()
{
	//LogMsgWithRole("AHeroCharacter::StopWeaponFire");

	if (bWantsToFire)
	{
		bWantsToFire = false;

		GetWorld()->GetTimerManager().ClearTimer(RunEndTimerHandle);

		if (GetCurrentWeapon())
		{
			GetCurrentWeapon()->Input_ReleaseTrigger();
		}
	}
}

bool AHeroCharacter::IsFiring() const
{
	return bWantsToFire;
};




// Inventory - Getters

AWeapon* AHeroCharacter::GetWeapon(EInventorySlots Slot) const
{
	switch (Slot)
	{
	case EInventorySlots::Primary: return PrimaryWeaponSlot;
	case EInventorySlots::Secondary: return SecondaryWeaponSlot;

	default:
		return nullptr;
	}
}
AItemBase* AHeroCharacter::GetItem(EInventorySlots Slot) const
{
	switch (Slot)
	{
	case EInventorySlots::Health: return GetFirstHealthItemOrNull();
	case EInventorySlots::Armour: return GetFirstArmourItemOrNull();
		//case EInventorySlots::Secondary: return SecondaryWeaponSlot;

	default:
		return nullptr;
	}
}
IEquippable* AHeroCharacter::GetEquippable(EInventorySlots Slot) const
{
	switch (Slot)
	{
	case EInventorySlots::Primary: return PrimaryWeaponSlot;
	case EInventorySlots::Secondary: return SecondaryWeaponSlot;
	case EInventorySlots::Health: return GetFirstHealthItemOrNull();
	case EInventorySlots::Armour: return GetFirstArmourItemOrNull();

	case EInventorySlots::Undefined:
	default:;
	}

	return nullptr;
}

int AHeroCharacter::GetHealthItemCount() const
{
	return HealthSlot.Num();
}
int AHeroCharacter::GetArmourItemCount() const
{
	return ArmourSlot.Num();
}

AWeapon* AHeroCharacter::GetCurrentWeapon() const
{
	return GetWeapon(CurrentInventorySlot);
}
AItemBase* AHeroCharacter::GetCurrentItem() const
{
	return GetItem(CurrentInventorySlot);
}
IEquippable* AHeroCharacter::GetCurrentEquippable() const
{
	return GetEquippable(CurrentInventorySlot);
}

bool AHeroCharacter::HasAnItemEquipped() const
{
	auto Equippable = GetEquippable(CurrentInventorySlot);
	if (!Equippable) return false;
	return Equippable->Is(EInventoryCategory::Health) || Equippable->Is(EInventoryCategory::Armour);
}
bool AHeroCharacter::HasAWeaponEquipped() const
{
	auto Equippable = GetEquippable(CurrentInventorySlot);
	if (!Equippable) return false;
	return Equippable->Is(EInventoryCategory::Weapon);// || Equippable->Is(EInventoryCategory::Throwable);
}


// Inventory - Spawning

void AHeroCharacter::GiveItemToPlayer(TSubclassOf<AItemBase> ItemClass)
{
	LogMsgWithRole("AHeroCharacter::GiveItemToPlayer");

	check(HasAuthority() && GetWorld() && ItemClass)


		// Spawn the item at the hand socket
		const auto TF = GetMesh()->GetSocketTransform(HandSocketName, RTS_World);
		auto* Item = GetWorld()->SpawnActorDeferred<AItemBase>(
		ItemClass,
		TF,
		this,
		this,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	UGameplayStatics::FinishSpawningActor(Item, TF);

	Item->SetHidden(true);

	// Find correct slot
	auto Slot = EInventorySlots::Undefined;

	switch (Item->GetInventoryCategory()) {
	case EInventoryCategory::Health: Slot = EInventorySlots::Health; break;
	case EInventoryCategory::Armour: Slot = EInventorySlots::Armour; break;
	case EInventoryCategory::Weapon: break;
	case EInventoryCategory::Undefined: break;
	case EInventoryCategory::Throwable: break;
	default:;
	}


	// Assign Item to inventory slot - TODO Combine this with the Assign weapon to slot code
	switch (Slot) {

	case EInventorySlots::Health:
	{
		// Add to inv
		HealthSlot.Add(Item);
		Item->EnterInventory();
		Item->SetRecipient(this);
		Item->SetDelegate(this);
	}
	break;

	case EInventorySlots::Armour:
	{
		// Add to inv
		ArmourSlot.Add(Item);
		Item->EnterInventory();
		Item->SetRecipient(this);
		Item->SetDelegate(this);
	}
	break;

	case EInventorySlots::Undefined: break;
	case EInventorySlots::Primary: break;
	case EInventorySlots::Secondary: break;
	default:;
	}


	// Put it in our hands! TODO - Or not?
	//EquipSlot(Slot);

	//LogMsgWithRole("AHeroCharacter::GiveItemToPlayer2");
}

AItemBase* AHeroCharacter::GetFirstHealthItemOrNull() const
{
	return HealthSlot.Num() > 0 ? HealthSlot[0] : nullptr;
}
AItemBase* AHeroCharacter::GetFirstArmourItemOrNull() const
{
	return ArmourSlot.Num() > 0 ? ArmourSlot[0] : nullptr;
}

void AHeroCharacter::GiveWeaponToPlayer(TSubclassOf<class AWeapon> WeaponClass, FWeaponConfig& Config)
{
	const auto Weapon = AuthSpawnWeapon(WeaponClass, Config);
	const auto Slot = FindGoodWeaponSlot();
	const auto RemovedWeapon = AssignWeaponToInventorySlot(Weapon, Slot);

	if (RemovedWeapon)
	{
		// If it's the same slot, replay the equip weapon
		RemovedWeapon->ExitInventory();

		// Drop weapon on ground
		TArray<AWeapon*> WeaponArray{ RemovedWeapon };
		SpawnWeaponPickups(WeaponArray);

		RemovedWeapon->Destroy();
		CurrentInventorySlot = EInventorySlots::Undefined;
	}

	EquipSlot(Slot);
}
AWeapon* AHeroCharacter::AuthSpawnWeapon(TSubclassOf<AWeapon> weaponClass, FWeaponConfig& Config)
{
	//LogMsgWithRole("AHeroCharacter::ServerRPC_SpawnWeapon");
	check(HasAuthority())
	if (!GetWorld()) return nullptr;


	const auto TF = GetMesh()->GetSocketTransform(HandSocketName, RTS_World);

	// Spawn the weapon at the weapon socket
	auto* Weapon = GetWorld()->SpawnActorDeferred<AWeapon>(
		weaponClass,
		TF,
		this,
		this,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!Weapon)
	{
		UE_LOG(LogTemp, Error, TEXT("AHeroCharacter::AuthSpawnWeapon - Failed to spawn weapon"));
		return nullptr;
	}

	Weapon->ConfigWeapon(Config);
	Weapon->SetHeroControllerId(GetHeroController()->PlayerState->PlayerId);

	UGameplayStatics::FinishSpawningActor(Weapon, TF);

	return Weapon;
}
EInventorySlots AHeroCharacter::FindGoodWeaponSlot() const
{
	// Find an empty slot, if one exists
	if (!PrimaryWeaponSlot) return EInventorySlots::Primary;
	if (!SecondaryWeaponSlot) return EInventorySlots::Secondary;

	// If we currently have a weapon selected, replace that.
	if (CurrentInventorySlot == EInventorySlots::Primary || CurrentInventorySlot == EInventorySlots::Secondary)
		return CurrentInventorySlot;

	// If the last inventory slot was a weapon, use that
	if (LastInventorySlot == EInventorySlots::Primary || LastInventorySlot == EInventorySlots::Secondary)
		return LastInventorySlot;

	return EInventorySlots::Primary; // just default to the first slot
}
AWeapon* AHeroCharacter::AssignWeaponToInventorySlot(AWeapon* Weapon, EInventorySlots Slot)
{
	bool IsNotAWeaponSlot = Slot != EInventorySlots::Primary && Slot != EInventorySlots::Secondary;
	if (IsNotAWeaponSlot)
	{
		UE_LOG(LogTemp, Error, TEXT("Attempting to equip a weapon into a non weapon slot: %d"), Slot);
		return nullptr;
	}

	const auto ToRemove = GetWeapon(Slot);

	//SetWeapon(Weapon, Slot);
	// This does not free up resources by design! If needed, first get the weapon before overwriting it
	if (Slot == EInventorySlots::Primary) PrimaryWeaponSlot = Weapon;
	if (Slot == EInventorySlots::Secondary) SecondaryWeaponSlot = Weapon;
	Weapon->EnterInventory();

	//// Cleanup previous weapon // TODO Drop this on ground
	//if (Removed) Removed->Destroy();
	return ToRemove;
}
bool AHeroCharacter::RemoveEquippableFromInventory(IEquippable* Equippable)
{
	check(Equippable);

	// TODO Create a collection of items to be cleared out each tick after a second of chillin out. Just to give things a chance to settle

	auto bMustChangeSlot = false;
	bool WasRemoved = false;


	if (Equippable->GetInventoryCategory() == EInventoryCategory::Health)
	{
		const auto Index = HealthSlot.IndexOfByPredicate([Equippable](AItemBase* Item)
		{
			return Item == Equippable;
		});

		if (Index != INDEX_NONE)
		{
			Equippable->ExitInventory();
			HealthSlot.RemoveAt(Index);
			WasRemoved = true;

			bMustChangeSlot = HealthSlot.Num() == 0 && CurrentInventorySlot == EInventorySlots::Health;
		}
	}



	if (Equippable->GetInventoryCategory() == EInventoryCategory::Armour)
	{
		const auto Index = ArmourSlot.IndexOfByPredicate([Equippable](AItemBase* Item)
			{
				return Item == Equippable;
			});

		if (Index != INDEX_NONE)
		{
			Equippable->ExitInventory();
			ArmourSlot.RemoveAt(Index);
			WasRemoved = true;

			bMustChangeSlot = ArmourSlot.Num() == 0 && CurrentInventorySlot == EInventorySlots::Armour;
		}
	}

	if (bMustChangeSlot)
	{
		const auto NewSlot = LastInventorySlot != CurrentInventorySlot ? LastInventorySlot : EInventorySlots::Primary;
		EquipSlot(NewSlot);
	}

	return WasRemoved;
}


// Inventory - Equipping

void AHeroCharacter::EquipSlot(const EInventorySlots Slot)
{
	// Already selected?
	if (CurrentInventorySlot == Slot) return;

	// Desired slot is empty?
	auto NewEquippable = GetEquippable(Slot);
	if (!NewEquippable) return;


	LastInventorySlot = CurrentInventorySlot;
	CurrentInventorySlot = Slot;


	// TODO Some delay on holster
	

	// Clear any existing Equip timer
	GetWorld()->GetTimerManager().ClearTimer(EquipTimerHandle);


	// Unequip old 
	auto OldEquippable = GetEquippable(LastInventorySlot);
	if (OldEquippable)
	{
		//LogMsgWithRole("Un-equip new slot");
		OldEquippable->Unequip();
		OldEquippable->SetHidden(OldEquippable->ShouldHideWhenUnequipped());
	}


	// Equip new
	if (NewEquippable)
	{
		//LogMsgWithRole("Equip new slot");
		NewEquippable->Equip();
		NewEquippable->SetHidden(true);

		GetWorldTimerManager().SetTimer(EquipTimerHandle, this, &AHeroCharacter::MakeEquippedItemVisible, NewEquippable->GetEquipDuration(), false);
	}

	RefreshWeaponAttachments();
}
void AHeroCharacter::MakeEquippedItemVisible() const
{
	//LogMsgWithRole("MakeEquippedItemVisible");
	IEquippable* Item = GetEquippable(CurrentInventorySlot);

	if (Item) Item->SetHidden(false);

	RefreshWeaponAttachments();
}
void AHeroCharacter::RefreshWeaponAttachments() const
{
	const FAttachmentTransformRules Rules{ EAttachmentRule::SnapToTarget, true };

	
	// Attach weapons to the correct locations
	auto W1 = GetWeapon(EInventorySlots::Primary);
	auto W2 = GetWeapon(EInventorySlots::Secondary);

	if (CurrentInventorySlot == EInventorySlots::Primary)
	{
		if (W1) W1->AttachToComponent(GetMesh(), Rules, HandSocketName);
		if (W2) W2->AttachToComponent(GetMesh(), Rules, Holster2SocketName);
	}
	else if (CurrentInventorySlot == EInventorySlots::Secondary)
	{
		if (W1) W1->AttachToComponent(GetMesh(), Rules, Holster1SocketName);
		if (W2) W2->AttachToComponent(GetMesh(), Rules, HandSocketName);
	}
	else
	{
		if (W1) W1->AttachToComponent(GetMesh(), Rules, Holster1SocketName);
		if (W2) W2->AttachToComponent(GetMesh(), Rules, Holster2SocketName);
	}

	if (CurrentInventorySlot == EInventorySlots::Health || CurrentInventorySlot == EInventorySlots::Armour)
	{
		auto Item = GetItem(CurrentInventorySlot);
		if (Item)
		{
			Item->AttachToComponent(GetMesh(), Rules, HandSocketName);
			Item->SetHidden(false); // This means it'll be visible even it's mid equip. 
		}

	}
}

void AHeroCharacter::NotifyItemIsExpended(AItemBase* Item)
{
	//LogMsgWithRole("AHeroCharacter::NotifyEquippableIsExpended()");
	check(HasAuthority())

	auto WasRemoved = RemoveEquippableFromInventory(Item);
	if (WasRemoved)
	{
		Item->Destroy();
		RefreshWeaponAttachments();
	}
}


// Inventory - Dropping

void AHeroCharacter::SpawnHeldWeaponsAsPickups() const
{
	check(HasAuthority());

	// Gather all weapons to drop
	TArray<AWeapon*> WeaponsToDrop{};
	const auto W1 = GetWeapon(EInventorySlots::Primary);
	const auto W2 = GetWeapon(EInventorySlots::Secondary);
	if (W1) WeaponsToDrop.Add(W1);
	if (W2) WeaponsToDrop.Add(W2);

	SpawnWeaponPickups(WeaponsToDrop);
}

void AHeroCharacter::SpawnWeaponPickups(TArray<AWeapon*> & Weapons) const
{
	// Gather the Pickup Class types to spawn
	TArray<TTuple<TSubclassOf<AWeaponPickupBase>, FWeaponConfig>> PickupClassesToSpawn{};
	int Count = 0;
	
	for (auto W : Weapons)
	{
		if (W && W->PickupClass && W->HasAmmo())
		{
			// Spawn location algorithm: Alternative between in front and behind player location
			const int FacingFactor = Count % 2 == 0 ? 1 : -1;
			auto Loc = GetActorLocation() + GetActorForwardVector() * 30 * FacingFactor;

			auto Params = FActorSpawnParameters{};
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			auto WeaponPickup = GetWorld()->SpawnActor<AWeaponPickupBase>(W->PickupClass, Loc, FRotator{}, Params);
			if (WeaponPickup)
			{
				WeaponPickup->SetWeaponConfig(FWeaponConfig{ W->GetAmmoInClip(), W->GetAmmoInPool() });
				WeaponPickup->bIsSingleUse = true;
				WeaponPickup->SetLifeSpan(60);
			
				Count++;
			}
		}
	}
}


// Camera tracks aim

void AHeroCharacter::MoveCameraByOffsetVector(const FVector2D& OffsetVec, float DeltaSeconds) const
{
	// Calculate a world space offset based on LeanVector
	const FVector Offset_WorldSpace = FVector{OffsetVec.Y, OffsetVec.X, 0.f};

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
		             ? 1 // Trying no cushion //LeanCushionRateMouse * DeltaSeconds
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
	FVector2D LinearLeanVector = FVector2D{AxisFaceRight, AxisFaceUp};
	return LinearLeanVector;
}

void AHeroCharacter::ExperimentalMouseAimTracking(float DT)
{
	auto* const Hero = GetHeroController();
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
	Controller->SetControlRotation((AimVec * -1).Rotation());
	// reverse aim vec just to help see setcontrolrotation works here


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
	return CursorVecRelative * FVector2D{1, -1};
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

bool AHeroCharacter::CanGiveHealth()
{
	return Health < MaxHealth;
}

bool AHeroCharacter::AuthTryGiveHealth(float Hp)
{
	//LogMsgWithRole("TryGiveHealth");
	if (!HasAuthority()) return false;

	if (!CanGiveHealth()) return false;

	Health = FMath::Min(Health + Hp, MaxHealth);
	return true;
}

bool AHeroCharacter::CanGiveAmmo()
{
	return FindWeaponToReceiveAmmo() != nullptr;
}
AWeapon* AHeroCharacter::FindWeaponToReceiveAmmo() const
{
	// Try give ammo to equipped weapon
	AWeapon* CurrentWeapon = GetCurrentWeapon();
	if (CurrentWeapon && CurrentWeapon->CanGiveAmmo())
	{
		return CurrentWeapon; // ammo given to main weapon
	}

	// TODO Give ammo to current weapon, if that fails give it to the other slot. If no weapon is equipped, give it to the first holstered gun found


	// Try give ammo to alternate weapon!
	AWeapon* AltWeap = nullptr;
	if (CurrentInventorySlot == EInventorySlots::Primary) AltWeap = GetWeapon(EInventorySlots::Secondary);
	if (CurrentInventorySlot == EInventorySlots::Secondary) AltWeap = GetWeapon(EInventorySlots::Primary);
	if (AltWeap && AltWeap->CanGiveAmmo() && AltWeap->TryGiveAmmo())
	{
		return AltWeap; // ammo given to alt weapon
	}

	return nullptr;
}
bool AHeroCharacter::AuthTryGiveAmmo()
{
	//LogMsgWithRole("AHeroCharacter::TryGiveAmmo");
	if (!HasAuthority()) return false;

	auto Weap = FindWeaponToReceiveAmmo();
	if (Weap) 
		return Weap->TryGiveAmmo();

	return false;
}


bool AHeroCharacter::CanGiveArmour()
{
	return Armour < MaxArmour;
}

bool AHeroCharacter::AuthTryGiveArmour(float Delta)
{
	//LogMsgWithRole("TryGiveArmour");
	if (!HasAuthority()) return false;
	if (!CanGiveArmour()) return false;

	Armour = FMath::Min(Armour + Delta, MaxArmour);
	return true;
}

bool AHeroCharacter::AuthTryGiveWeapon(const TSubclassOf<AWeapon>& Class, FWeaponConfig& Config)
{
	check(HasAuthority());
	check(Class != nullptr);

	//LogMsgWithRole("AHeroCharacter::TryGiveWeapon");

	float OutDelay;
	if (!CanGiveWeapon(Class, OUT OutDelay)) return false;

	GiveWeaponToPlayer(Class, Config);
	return true;
}

bool AHeroCharacter::CanGiveWeapon(const TSubclassOf<AWeapon>& Class, OUT float& OutDelay)
{
	EInventorySlots GoodSlot = FindGoodWeaponSlot();

	// Get pickup delay
	const bool bIdealSlotAlreadyContainsWeapon = GetWeapon(GoodSlot) != nullptr;
	OutDelay = bIdealSlotAlreadyContainsWeapon ? 2 : 0;
	return true;
}

bool AHeroCharacter::CanGiveItem(const TSubclassOf<AItemBase>& Class, float& OutDelay)
{
	LogMsgWithRole("AHeroCharacter::CanGiveItem");
	OutDelay = 0;

	// Health and Armour items have limits. Check if we're under those limits

	const int HealthCount = HealthSlot.Num();
	const int ArmourCount = ArmourSlot.Num();

	// If we certainly have space, lets go!
	if (HealthCount < HealthSlotLimit && ArmourCount < ArmourSlotLimit)
		return true;


	// Need to create an instance to see what category it is
	auto Temp = NewObject<AItemBase>(this, Class);
	const auto Category = Temp->GetInventoryCategory();
	Temp->Destroy();

	if (Category == EInventoryCategory::Health && HealthCount == HealthSlotLimit)
		return false;

	if (Category == EInventoryCategory::Armour && ArmourCount == ArmourSlotLimit)
		return false;

	return true;
}

bool AHeroCharacter::TryGiveItem(const TSubclassOf<AItemBase>& Class)
{
	LogMsgWithRole("AHeroCharacter::TryGiveItem");

	check(HasAuthority());
	check(Class != nullptr);

	//LogMsgWithRole("AHeroCharacter::TryGiveWeapon");

	float OutDelay;
	if (!CanGiveItem(Class, OUT OutDelay)) return false;

	GiveItemToPlayer(Class);
	return true;
}

FTransform AHeroCharacter::GetAimTransform() const
{
	const auto W = GetCurrentWeapon();

	if (!W)
	{
		// No weapon equipped, just use the weapon anchor for now.
		return WeaponAnchor->GetComponentTransform();
	}

	// Detect current weapon and use the barrel tform in local space to provide an initial location for aiming/shooting to orient from, but doesn't change based on animation.

	// Using weapon Anchor as a base because it doesn't move with animations
	const auto SocketTform = WeaponAnchor->GetComponentTransform();

	// Get Weapon Barrel Relative offset from the hand
	const auto RelativeMuzzleTform = W->GetMuzzleComponent()->GetRelativeTransform();

	// Combine the transforms to get a decent approximation of where the barrel is, minus the animation movement.
	const auto CombinedTform = RelativeMuzzleTform * SocketTform;

	// Return it!
	return CombinedTform;
}


// Interacting

void AHeroCharacter::Input_Interact()
{
	//LogMsgWithRole("AHeroCharacter::Input_Interact()");
	ServerRPC_TryInteract();
}
void AHeroCharacter::ServerRPC_TryInteract_Implementation()
{
	//LogMsgWithRole("AHeroCharacter::ServerRPC_TryInteract_Implementation()");

	auto* const Pickup = ScanForInteractable<APickupBase>();

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
	if (bDrawDebug) DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor{255, 0, 0}, false, -1., 0, 3.f);

	// Raycast along line to find intersecting physics object
	FHitResult hitResult;
	bool isHit = GetWorld()->LineTraceSingleByObjectType( // TODO Convert to 
		OUT hitResult,
		traceStart,
		traceEnd,
		FCollisionObjectQueryParams{ECollisionChannel::ECC_GameTraceChannel2},
		FCollisionQueryParams{FName(""), false, GetOwner()}
	);

	if (isHit && bDrawDebug)
	{
		DrawDebugLine(GetWorld(), traceStart, hitResult.ImpactPoint, FColor{0, 0, 255}, false, -1., 0, 5.f);
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

float AHeroCharacter::GetTargetingSpeedModifier() const
{
	return GetCurrentWeapon() ? GetCurrentWeapon()->GetAdsMovementScale() : 1;
}

bool AHeroCharacter::IsReloading() const
{
	const auto W = GetCurrentWeapon();
	if (W && W->IsReloading()) return true;

	return false;
}

bool AHeroCharacter::IsUsingItem() const
{
	const auto Item = GetCurrentItem();
	if (Item && Item->IsInUse()) return true;

	return false;
}

bool AHeroCharacter::IsBackpedaling(const FVector& MoveDir, const FVector& AimDir, int BackpedalAngle)
{
	const bool bBackpedaling = FVector::DotProduct(MoveDir, AimDir) < FMath::
		Cos(FMath::DegreesToRadians(BackpedalAngle));

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
	switch (role)
	{
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
