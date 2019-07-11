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
			GiveWeaponToPlayer(DefaultWeaponClass[Choice]);
		}
	}
}

void AHeroCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHeroCharacter, CurrentInventorySlot);
	DOREPLIFETIME(AHeroCharacter, PrimaryWeaponSlot);
	DOREPLIFETIME(AHeroCharacter, SecondaryWeaponSlot);
	DOREPLIFETIME(AHeroCharacter, Health);
	DOREPLIFETIME(AHeroCharacter, Armour);
	DOREPLIFETIME(AHeroCharacter, TeamTint);
	//	DOREPLIFETIME(AHeroCharacter, bIsAdsing);

	// everyone except local owner: flag change is locally instigated
	DOREPLIFETIME_CONDITION(AHeroCharacter, bIsRunning, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AHeroCharacter, bIsTargeting, COND_SkipOwner);

	// Just the owner
	DOREPLIFETIME_CONDITION(AHeroCharacter, LastInventorySlot, COND_OwnerOnly);

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
		if (World)
		{
			auto Str = FString::Printf(TEXT("Grab (%s)"), *ActionText.ToString());
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
		ScanForWeaponPickups(DeltaSeconds);
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
		if (GetVelocity().Size() < 100)
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
	PlayerInputComponent->BindAction("PrimaryWeapon", IE_Pressed, this, &AHeroCharacter::OnEquipHealth);

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
	// TODO Ask inventory to equip health

	// TODO Return if health already equipped or zero health items

	EquipSlot(EInventorySlots::Health);
}
void AHeroCharacter::ServerEquipHealth_Implementation()
{
	EquipHealth();
}
bool AHeroCharacter::ServerEquipHealth_Validate()
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

	//	bUseControllerRotationYaw = false;
	//	GetCharacterMovement()->bOrientRotationToMovement = true;


		if (bCancelReloadOnRun && GetCurrentWeapon()) GetCurrentWeapon()->CancelAnyReload();
	}
	else // Is Walking 
	{
		// Config movement properties TODO Make these data driven (BP) and use Meaty char movement comp
		GetCharacterMovement()->MaxAcceleration = 3000;
		GetCharacterMovement()->BrakingFrictionFactor = 2;
		//GetCharacterMovement()->bUseSeparateBrakingFriction = false;
		GetCharacterMovement()->BrakingDecelerationWalking = 3000;

	//	bUseControllerRotationYaw = true;;
	//	GetCharacterMovement()->bOrientRotationToMovement = false;


		LastRunEnded = FDateTime::Now();
	}

	

	if (Role < ROLE_Authority)
	{
		ServerSetRunning(bNewIsRunning);
	}
}

bool AHeroCharacter::IsRunning() const
{
	return bIsRunning;

	//FVector Velocity = GetVelocity().GetSafeNormal2D();
	//FVector Facing = GetActorForwardVector();

	//bool bIsRunning = bIsRunning && !GetVelocity().IsZero() 
	//	&& (Velocity | Facing) > FMath::Cos(FMath::DegreesToRadians(SprintMaxAngle));

	//// Debug
	//if (false && bIsRunning)
	//{
	//	const float Angle = FMath::RadiansToDegrees(FMath::Acos(Velocity | Facing));
	//	UE_LOG(LogTemp, Warning, TEXT("Sprinting! %fd"), Angle);
	//}

	//return bIsRunning;
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

void AHeroCharacter::Input_FirePressed()
{
	auto* MyPC = Cast<AHeroController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (bIsRunning || IsRunning())
		{
			SetRunning(false);
		}
		StartWeaponFire();
	}
}

void AHeroCharacter::Input_FireReleased()
{
	StopWeaponFire();
	//if (GetCurrentWeapon()) GetCurrentWeapon()->Input_ReleaseTrigger();
}

void AHeroCharacter::Input_AdsPressed()
{
	auto* MyPC = Cast<AHeroController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (bIsRunning || IsRunning())
		{
			SetRunning(false);
		}
		SetTargeting(true);
	}
}

void AHeroCharacter::Input_AdsReleased()
{
	//SimulateAdsMode(false);
	//ServerRPC_AdsReleased();
	SetTargeting(false);
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




IEquippable* AHeroCharacter::GetEquippable(EInventorySlots Slot) const
{
	switch (Slot) 
	{ 
		case EInventorySlots::Primary: return PrimaryWeaponSlot;
		case EInventorySlots::Secondary: return SecondaryWeaponSlot;
		case EInventorySlots::Health: return HealthSlot;

		case EInventorySlots::Undefined:
		default:;
	}

	return nullptr;
}



// Weapon spawning

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

AWeapon* AHeroCharacter::GetCurrentWeapon() const
{
	return GetWeapon(CurrentInventorySlot);
}

//AWeapon* AHeroCharacter::GetHolsteredWeapon() const
//{
//	if (CurrentWeaponSlot == EWeaponSlots::Primary) return GetWeapon(EWeaponSlots::Secondary);
//	if (CurrentWeaponSlot == EWeaponSlots::Secondary) return GetWeapon(EWeaponSlots::Primary);
//	return nullptr;
//}


void AHeroCharacter::GiveWeaponToPlayer(TSubclassOf<class AWeapon> WeaponClass)
{
	const auto Weapon = AuthSpawnWeapon(WeaponClass);
	const auto Slot = FindGoodWeaponSlot();
	const auto RemovedWeapon = AssignWeaponToInventorySlot(Weapon, Slot);

	if (RemovedWeapon)
	{
		// If it's the same slot, replay the equip weapon
		RemovedWeapon->Destroy();
		CurrentInventorySlot = EInventorySlots::Undefined;
	}

	EquipSlot(Slot);
}

AWeapon* AHeroCharacter::AuthSpawnWeapon(TSubclassOf<AWeapon> weaponClass)
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

	//if (Weapon == nullptr) { return nullptr; }


	//// Configure it
	//auto rules = FAttachmentTransformRules{ EAttachmentRule::KeepRelative, true };
	//Weapon->AttachToComponent(
	//	GetMesh(), 
	//	rules,
	//	SocketName);

	Weapon->SetHeroControllerId(GetHeroController()->PlayerState->PlayerId);


	// Finish him!
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

	const auto Removed = GetWeapon(Slot);

	//SetWeapon(Weapon, Slot);
	// This does not free up resources by design! If needed, first get the weapon before overwriting it
	if (Slot == EInventorySlots::Primary) PrimaryWeaponSlot = Weapon;
	if (Slot == EInventorySlots::Secondary) SecondaryWeaponSlot = Weapon;


	//// Cleanup previous weapon // TODO Drop this on ground
	//if (Removed) Removed->Destroy();
	return Removed;
}

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
		OldEquippable->SetHidden(false);
	}


	// Equip new
	if (NewEquippable)
	{
		//LogMsgWithRole("Equip new slot");
		NewEquippable->Equip();
		const float DrawDuration = NewEquippable->GetEquipDuration();
		NewEquippable->SetHidden(true);

		GetWorldTimerManager().SetTimer(EquipTimerHandle, this, &AHeroCharacter::MakeEquippedItemVisible, DrawDuration, false);
	}

	RefreshWeaponAttachments();
}


void AHeroCharacter::MakeEquippedItemVisible() const
{
	LogMsgWithRole("MakeEquippedItemVisible");
	IEquippable* Item = GetEquippable(CurrentInventorySlot);

	if (Item) Item->SetHidden(false);

	RefreshWeaponAttachments();
}

void AHeroCharacter::RefreshWeaponAttachments() const
{
	const FAttachmentTransformRules Rules{ EAttachmentRule::SnapToTarget, true };

	auto W1 = GetWeapon(EInventorySlots::Primary);
	auto W2 = GetWeapon(EInventorySlots::Secondary);


	/*if (IsRunning())
	{
		if (W1) W1->AttachToComponent(GetMesh(), Rules, Holster1SocketName);
		if (W2) W2->AttachToComponent(GetMesh(), Rules, Holster2SocketName);
	}
	else*/ // Is Normal
	{
		if (CurrentInventorySlot == EInventorySlots::Undefined)
		{
			if (W1) W1->AttachToComponent(GetMesh(), Rules, Holster1SocketName);
			if (W2) W2->AttachToComponent(GetMesh(), Rules, Holster2SocketName);
		}

		if (CurrentInventorySlot == EInventorySlots::Primary)
		{
			if (W1) W1->AttachToComponent(GetMesh(), Rules, HandSocketName);
			if (W2) W2->AttachToComponent(GetMesh(), Rules, Holster2SocketName);
		}

		if (CurrentInventorySlot == EInventorySlots::Secondary)
		{
			if (W1) W1->AttachToComponent(GetMesh(), Rules, Holster1SocketName);
			if (W2) W2->AttachToComponent(GetMesh(), Rules, HandSocketName);
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

bool AHeroCharacter::AuthTryGiveHealth(float Hp)
{
	//LogMsgWithRole("TryGiveHealth");
	if (!HasAuthority()) return false;

	if (Health == MaxHealth) return false;

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
	if (AltWeap && AltWeap->TryGiveAmmo())
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

	float OutDelay;
	if (!CanGiveWeapon(Class, OUT OutDelay)) return false;

	GiveWeaponToPlayer(Class);
	return true;
}

bool AHeroCharacter::CanGiveWeapon(const TSubclassOf<AWeapon>& Class, OUT float& OutDelay)
{
	EInventorySlots GoodSlot = FindGoodWeaponSlot();

	// Get pickup delay
	const bool bIdealSlotAlreadyContainsWeapon = GetWeapon(GoodSlot) != nullptr;
	OutDelay = bIdealSlotAlreadyContainsWeapon ? 2 : 0;


	//// Dont pickup the weapon if it's the same as the one we're already holding
	//if (GoodSlot == CurrentWeaponSlot && GetCurrentWeapon() && GetCurrentWeapon()->IsA(Class))
	//{
	//	return false; // Don't pick up weapon of same type
	//}

	// Always allow pickup of weapon - for now
	return true;
}

bool AHeroCharacter::CanGiveItem(const TSubclassOf<AItemBase>& Class, float& OutDelay)
{
	LogMsgWithRole("Can give item!");
	return true;
}

bool AHeroCharacter::TryGiveItem(const TSubclassOf<AItemBase>& Class)
{
	LogMsgWithRole("Item get!");
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


void AHeroCharacter::ServerRPC_EquipPrimaryWeapon_Implementation()
{
	EquipSlot(EInventorySlots::Primary);
}

bool AHeroCharacter::ServerRPC_EquipPrimaryWeapon_Validate()
{
	return true;
}

void AHeroCharacter::ServerRPC_EquipSecondaryWeapon_Implementation()
{
	EquipSlot(EInventorySlots::Secondary);
}

bool AHeroCharacter::ServerRPC_EquipSecondaryWeapon_Validate()
{
	return true;
}


void AHeroCharacter::ServerRPC_ToggleWeapon_Implementation()
{
	switch (CurrentInventorySlot)
	{
	case EInventorySlots::Primary:
		EquipSlot(EInventorySlots::Secondary);
		break;

	case EInventorySlots::Undefined:
	case EInventorySlots::Secondary:
	default:
		EquipSlot(EInventorySlots::Primary);
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
