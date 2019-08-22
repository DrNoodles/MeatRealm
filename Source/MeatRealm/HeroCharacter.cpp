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
#include "Projectile.h"

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

	InventoryComp = CreateDefaultSubobject<UInventoryComp>(TEXT("InventoryComp"));

	LastRunEnded = FDateTime::Now();
}

void AHeroCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ROLE_Authority == Role)
	{
		InventoryComp->DestroyInventory();
	}
}

void AHeroCharacter::Restart()
{
	Super::Restart();
	LogMsgWithRole("AHeroCharacter::Restart()");

	Health = MaxHealth;
	Armour = 0.f;

	InventoryComp->InitInventory();
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

	AWeapon* CurrentWeapon = InventoryComp->GetCurrentWeapon();
	if (CurrentWeapon && CurrentWeapon->IsEquipping())
	{
		auto str = FString::Printf(TEXT("Equipping %s"), *CurrentWeapon->GetWeaponName());
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
	auto Item = InventoryComp->GetCurrentItem();
	if (Item) Item->UsePressed();
}
void AHeroCharacter::UseItemReleased() const
{
	//LogMsgWithRole("AHeroCharacter::UseItemReleased");
	auto Item = InventoryComp->GetCurrentItem();
	if (Item) Item->UseReleased();
}
void AHeroCharacter::UseItemCancelled() const
{
	//LogMsgWithRole("AHeroCharacter::UseItemCancelled");
	auto Item = InventoryComp->GetCurrentItem();
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
	auto Item = InventoryComp->GetCurrentItem();

	if (InventoryComp->GetArmourItemCount() > 0 && CanGiveArmour())
	{
		if (Item && Item->IsAutoUseOnEquip() && Item->GetInventoryCategory() == EInventoryCategory::Armour)
		{
			Item->UsePressed();
		}
		else
		{
			InventoryComp->EquipSlot(EInventorySlots::Armour);
		}
	}
	else if (InventoryComp->GetHealthItemCount() > 0 && CanGiveHealth())
	{
		if (Item && Item->IsAutoUseOnEquip() && Item->GetInventoryCategory() == EInventoryCategory::Health)
		{
			Item->UsePressed();
		}
		else
		{
			InventoryComp->EquipSlot(EInventorySlots::Health);
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
	auto Item = InventoryComp->GetCurrentItem();
	if (Item && Item->IsAutoUseOnEquip() && Item->GetInventoryCategory() == EInventoryCategory::Health)
	{
		Item->UsePressed();
	}
	else
	{
		InventoryComp->EquipSlot(EInventorySlots::Health);
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
	auto Item = InventoryComp->GetCurrentItem();
	if (Item && Item->IsAutoUseOnEquip() && Item->GetInventoryCategory() == EInventoryCategory::Armour)
	{
		Item->UsePressed();
	}
	else
	{
		InventoryComp->EquipSlot(EInventorySlots::Armour);
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

	InventoryComp->EquipSlot(EInventorySlots::Primary);
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

	InventoryComp->EquipSlot(EInventorySlots::Secondary);
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

	if (InventoryComp->HasAWeaponEquipped())
	{
		if (InventoryComp->GetCurrentInventorySlot() == EInventorySlots::Primary)
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
		const auto LastInventorySlot = InventoryComp->GetLastInventorySlot();
		if (LastInventorySlot == EInventorySlots::Primary || LastInventorySlot == EInventorySlots::Secondary)
		{
			TargetSlot = LastInventorySlot;
		}
		else
		{
			TargetSlot = EInventorySlots::Primary; // Default to primary
		}
	}

	InventoryComp->EquipSlot(TargetSlot);
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

		auto CurrentWeapon = InventoryComp->GetCurrentWeapon();
		if (bCancelReloadOnRun && CurrentWeapon) CurrentWeapon->CancelAnyReload();
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


	auto CurrentWeapon = InventoryComp->GetCurrentWeapon();
	if (CurrentWeapon)
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
					const auto CurrentWeapon1 = InventoryComp->GetCurrentWeapon();
					if (bIsTargeting && CurrentWeapon1)
					{
						CurrentWeapon1->Input_AdsPressed();
					}
				};

				GetWorld()->GetTimerManager().SetTimer(RunEndTimerHandle, DelayAds, RemainingTime.GetTotalSeconds(), false);
			}
			else
			{
				CurrentWeapon->Input_AdsPressed();
			}
		}
		else
		{
			CurrentWeapon->Input_AdsReleased();
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
		if (InventoryComp->HasAnItemEquipped())
		{
			UseItemPressed();
		}
		else if (InventoryComp->HasAWeaponEquipped())
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
	if (InventoryComp->HasAnItemEquipped())
	{
		UseItemReleased();
	}
	else if (InventoryComp->HasAWeaponEquipped())
	{
		StopWeaponFire();
	}
}

void AHeroCharacter::Input_SecondaryPressed()
{
	auto* MyPC = Cast<AHeroController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (InventoryComp->HasAnItemEquipped())
		{
			UseItemCancelled();
		}
		else if (InventoryComp->HasAWeaponEquipped())
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
	if (InventoryComp->HasAWeaponEquipped())
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

		if (InventoryComp->GetCurrentWeapon())
		{
			InventoryComp->GetCurrentWeapon()->Input_Reload();
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
				if (bWantsToFire && InventoryComp->GetCurrentWeapon())
				{
					InventoryComp->GetCurrentWeapon()->Input_PullTrigger();
				}
			};

			GetWorld()->GetTimerManager().SetTimer(RunEndTimerHandle, DelayFire, RemainingTime.GetTotalSeconds(), false);
			return;
		}

		// Fire now!
		else if (InventoryComp->GetCurrentWeapon())
		{
			InventoryComp->GetCurrentWeapon()->Input_PullTrigger();
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

		if (InventoryComp->GetCurrentWeapon())
		{
			InventoryComp->GetCurrentWeapon()->Input_ReleaseTrigger();
		}
	}
}

bool AHeroCharacter::IsFiring() const
{
	return bWantsToFire;
};



void AHeroCharacter::RefreshWeaponAttachments()
{
	const FAttachmentTransformRules Rules{ EAttachmentRule::SnapToTarget, true };
	const auto CurrentInventorySlot = InventoryComp->GetCurrentInventorySlot();

	
	// Attach weapons to the correct locations
	auto W1 = InventoryComp->GetWeapon(EInventorySlots::Primary);
	auto W2 = InventoryComp->GetWeapon(EInventorySlots::Secondary);

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
		auto Item = InventoryComp->GetItem(CurrentInventorySlot);
		if (Item)
		{
			Item->AttachToComponent(GetMesh(), Rules, HandSocketName);
			Item->SetHidden(false); // This means it'll be visible even it's mid equip. 
		}
	}
}


//// Camera tracks aim /////////////////////////////////////////////////////

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




//// Affect the character /////////////////////////////////////////////////////

void AHeroCharacter::AuthOnDeath()
{
	check(HasAuthority());
	OnDeathImpl();

	// Inform clients
	MultiOnDeath();
}
void AHeroCharacter::MultiOnDeath_Implementation()
{
	if (HasAuthority()) return;
	
	OnDeathImpl();
}
bool AHeroCharacter::MultiOnDeath_Validate()
{
	return true;
}
void AHeroCharacter::OnDeathImpl()
{
	// Tweak networking
	NetUpdateFrequency = GetDefault<AHeroCharacter>()->NetUpdateFrequency;
	bReplicateMovement = false;
	TearOff();
	GetCharacterMovement()->ForceReplicationUpdate();

	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	if (HasAuthority())
	{
		InventoryComp->SpawnHeldWeaponsAsPickups();
		//InventoryComp->DestroyInventory();
	}

	// TODO Switch to 3rd person view of death

	DetachFromControllerPendingDestroy();



	if (GetMesh())
	{
		static FName CollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetCollisionProfileName(CollisionProfileName);
	}


	SetActorEnableCollision(true);

	SetRagdollPhysics();

	// disable collisions on capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);


	// TODO Add an impulse to fling the ragdoll
	auto fn = [&] {
		if (!IsActorBeingDestroyed())
			Destroy();

		//GetMesh()->AddRadialImpulse(FVector::ZeroVector, 10000, 1000, ERadialImpulseFalloff::RIF_Constant);
		//LaunchCharacter(FVector{ 10000,1000,1000 }, true, true);
	};

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, fn, 10, false);
}

void AHeroCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	InventoryComp->SetDelegate(this);
}

bool AHeroCharacter::ShouldTakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser) const
{
	return Super::ShouldTakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
}

uint32 AHeroCharacter::GetControllerId()
{
	const auto HeroController = GetHeroController();
	return HeroController ? HeroController->GetPlayerId() : -1;
}

FTransform AHeroCharacter::GetHandSocketTransform()
{
	return GetMesh()->GetSocketTransform(HandSocketName, RTS_World);
}

float AHeroCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	if (!HasAuthority()) return 0;
	
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	ActualDamage = FMath::RoundHalfFromZero(ActualDamage);
	if (ActualDamage < 1)
	{
		UE_LOG(LogTemp, Error, TEXT("HeroCharacter::TakeDamage() - Damage < 1!"))
		return 0;
	}
	
	LogMsgWithRole(FString::Printf(TEXT("AHeroCharacter::TakeDamage: %f"), ActualDamage));
	

	
	// Subtract vitality. Take from Armour first, then Health
	auto bHitArmour = false;
	if (Armour > 0)
	{
		bHitArmour = true;

		if (ActualDamage > Armour)
		{
			const float DamageToHealth = ActualDamage - Armour;
			Armour = 0;
			Health -= DamageToHealth;
		}
		else
		{
			Armour -= ActualDamage;
		}
	}
	else
	{
		Health -= ActualDamage;
	}

	LogMsgWithRole(FString::Printf(TEXT("%fhp %fap"), Health, Armour));


	

	auto VictimController = GetHeroController();


	const auto Projectile = Cast<AProjectile>(DamageCauser);
	if (Projectile == nullptr) 
		UE_LOG(LogTemp, Error, TEXT("AHeroCharacter - Projectile IS NULL! WTF"));
	
	// Report hit to controller
	if (VictimController)
	{
		FMRHitResult Hit{};
		Hit.VictimId = VictimController->GetPlayerId();
		Hit.AttackerId = Projectile ? Projectile->GetInstigatingControllerId() : -1;
		Hit.HealthRemaining = (int)Health;
		Hit.DamageTaken = (int)ActualDamage;
		Hit.bHitArmour = bHitArmour;
		Hit.HitLocation = GetActorLocation(); // TODO Get the actual location of the damage

		VictimController->TakeDamage2(Hit);
	}
	
	return ActualDamage;
}

void AHeroCharacter::SetRagdollPhysics()
{
	bool bInRagdoll = false;

	if (IsPendingKill())
	{
		bInRagdoll = false;
	}
	else if (!GetMesh() || !GetMesh()->GetPhysicsAsset())
	{
		bInRagdoll = false;
	}
	else
	{
		// initialize physics/etc
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;

		bInRagdoll = true;
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	if (!bInRagdoll)
	{
		// hide and set short lifespan
		TurnOff();
		SetActorHiddenInGame(true);
		//SetLifeSpan(1.0f);
	}
	else
	{
		//SetLifeSpan(10.0f);
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
	return InventoryComp->FindWeaponToReceiveAmmo() != nullptr;
}
bool AHeroCharacter::AuthTryGiveAmmo()
{
	//LogMsgWithRole("AHeroCharacter::TryGiveAmmo");
	if (!HasAuthority()) return false;

	auto Weap = InventoryComp->FindWeaponToReceiveAmmo();
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

	InventoryComp->GiveWeaponToPlayer(Class, Config);
	return true;
}
bool AHeroCharacter::CanGiveWeapon(const TSubclassOf<AWeapon>& Class, OUT float& OutDelay)
{
	EInventorySlots GoodSlot = InventoryComp->FindGoodWeaponSlot();

	// Get pickup delay
	const bool bIdealSlotAlreadyContainsWeapon = InventoryComp->GetWeapon(GoodSlot) != nullptr;
	OutDelay = bIdealSlotAlreadyContainsWeapon ? 2 : 0;
	return true;
}

bool AHeroCharacter::CanGiveItem(const TSubclassOf<AItemBase>& Class, float& OutDelay)
{
	OutDelay = 0;
	return InventoryComp->CanGiveItem(Class);
}
bool AHeroCharacter::TryGiveItem(const TSubclassOf<AItemBase>& Class)
{
	LogMsgWithRole("AHeroCharacter::TryGiveItem");

	check(HasAuthority());
	check(Class != nullptr);

	//LogMsgWithRole("AHeroCharacter::TryGiveWeapon");

	float OutDelay;
	if (!CanGiveItem(Class, OUT OutDelay)) return false;

	InventoryComp->GiveItemToPlayer(Class);
	return true;
}

FTransform AHeroCharacter::GetAimTransform() const
{
	const auto W = InventoryComp->GetCurrentWeapon();

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




//// Interacting //////////////////////////////////////////////////////////////

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
	const auto CurrentWeapon = InventoryComp->GetCurrentWeapon();	
	return CurrentWeapon ? CurrentWeapon->GetAdsMovementScale() : 1;
}

bool AHeroCharacter::IsReloading() const
{
	const auto W = InventoryComp->GetCurrentWeapon();
	if (W && W->IsReloading()) return true;

	return false;
}

bool AHeroCharacter::IsUsingItem() const
{
	const auto Item = InventoryComp->GetCurrentItem();
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
