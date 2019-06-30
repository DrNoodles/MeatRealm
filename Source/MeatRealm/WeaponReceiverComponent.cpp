#include "WeaponReceiverComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "UnrealNetwork.h"
#include "GameFramework/GameState.h"
#include "DrawDebugHelpers.h"

void UWeaponReceiverComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWeaponReceiverComponent, WeaponState);
}

UWeaponReceiverComponent::UWeaponReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bReplicates = true;
}

void UWeaponReceiverComponent::BeginPlay()
{
	Super::BeginPlay();

	WeaponState.AmmoInClip = ClipSizeGiven;
	WeaponState.AmmoInPool = AmmoPoolGiven;

	ChangeState(EWeaponCommands::Equip, WeaponState);
}



// Input state 

void UWeaponReceiverComponent::DrawWeapon()
{
	LogMsgWithRole(FString::Printf(TEXT("InputState.DrawRequested = true")));
	InputState.DrawRequested = true;
}
void UWeaponReceiverComponent::HolsterWeapon()
{
	LogMsgWithRole(FString::Printf(TEXT("InputState.HolsterRequested = true")));
	InputState.HolsterRequested = true;
}
void UWeaponReceiverComponent::PullTrigger()
{
	InputState.FireRequested = true;
	WeaponState.HasFired = false;
}
void UWeaponReceiverComponent::ReleaseTrigger()
{
	InputState.FireRequested = false;
	WeaponState.HasFired = false;
}
void UWeaponReceiverComponent::Reload()
{
	InputState.ReloadRequested = true;
}
void UWeaponReceiverComponent::AdsPressed()
{
	InputState.AdsRequested = true;
	WeaponState.IsAdsing = InputState.AdsRequested;
}
void UWeaponReceiverComponent::AdsReleased()
{
	InputState.AdsRequested = false;
	WeaponState.IsAdsing = InputState.AdsRequested;
}
bool UWeaponReceiverComponent::TryGiveAmmo()
{
	if (WeaponState.AmmoInPool == AmmoPoolSize) return false;

	WeaponState.AmmoInPool = FMath::Min(WeaponState.AmmoInPool + AmmoGivenPerPickup, AmmoPoolSize);

	return true;
}



// Tick and state transitions

void UWeaponReceiverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	
	// TODO These ticks might only do 1 operation per tick. Maybe return a bool from each TickFunction if a state was changed so we can reprocess it right away?
	if (HasAuthority())
	{
		switch (WeaponState.Mode)
		{
		case EWeaponModes::Idle:
			TickIdle(DeltaTime);
			break;

		case EWeaponModes::Firing:
			TickFiring(DeltaTime);
			break;

		case EWeaponModes::Reloading:
			TickReloading(DeltaTime);
			break;

		case EWeaponModes::UnEquipped:
			TickUnEquipped(DeltaTime);
			break;

		default:
			LogMsgWithRole(FString::Printf(TEXT("TickComponent() - WeaponMode unimplemented %d"), *EWeaponModesStr(WeaponState.Mode)));
		}
	}


	// Draw ADS line for self or others 
	// TODO Remove this from ReceiverComp, back into Weapon
	if (!HasAuthority() && WeaponState.IsAdsing)
	{
		const bool IsAutonomous = GetOwnerOwnerLocalRole() == ROLE_AutonomousProxy;
		const auto Color = IsAutonomous ? AdsLineColor : EnemyAdsLineColor;
		const auto Length = IsAutonomous ? AdsLineLength : EnemyAdsLineLength;
		DrawAdsLine(Color, Length);
	}
}

bool UWeaponReceiverComponent::TickIdle(float DT)
{
	//LogMsgWithRole("EWeaponModes::Ready");

	// Allow fluid enter/exit of ADS state
	//WeaponState.IsAdsing = InputState.AdsPressed;

	if (InputState.HolsterRequested)
	{
		LogMsgWithRole("EWeaponModes::TickReady - Processing HolsterRequested");
		InputState.HolsterRequested = false;
		return ChangeState(EWeaponCommands::UnEquip, WeaponState);
	}

	// Ready > Firing
	if (InputState.FireRequested)
	{
		LogMsgWithRole("EWeaponModes::TickReady - Processing FirePressed");
		return ChangeState(EWeaponCommands::FireStart, WeaponState);
	}


	// Ready > Reloading
	if (InputState.ReloadRequested)
	{
		LogMsgWithRole("EWeaponModes::TickReady - Processing ReloadRequested");
		// Only process for one frame as we dont have a release

		InputState.ReloadRequested = false;

		if (CanReload())
		{
			return ChangeState(EWeaponCommands::ReloadStart, WeaponState);
		}
	}

	return false;
}

bool UWeaponReceiverComponent::TickUnEquipped(float DeltaTime)
{
	auto str = FString::Printf(TEXT("EWeaponModes::TickPaused %s"), *WeaponState.ToString());
	//LogMsgWithRole(str);

	if (WeaponState.IsAdsing) WeaponState.IsAdsing = false;

	if (InputState.DrawRequested)
	{
		LogMsgWithRole("EWeaponModes::TickPaused - Processed Draw Request");
		InputState.DrawRequested = false;

		const auto Command = bIsMidReload ? EWeaponCommands::ReloadStart : EWeaponCommands::Equip;
		return ChangeState(Command, WeaponState);
	}

	return false;
}

bool UWeaponReceiverComponent::TickFiring(float DT)
{
	//LogMsgWithRole("EWeaponModes::Firing");

	if (InputState.HolsterRequested)
	{
		return ChangeState(EWeaponCommands::UnEquip, WeaponState);
	}



	if (bIsBusy) return false;
	const auto bReceiverCanCycle = bFullAuto || !WeaponState.HasFired;


	// Process State Transitions
	{
		if (!InputState.FireRequested)
		{ 
			return ChangeState(EWeaponCommands::FireEnd, WeaponState);
		}
		
		if (bReceiverCanCycle && NeedsReload() && CanReload())
		{
			// Reload instead
			return ChangeState(EWeaponCommands::ReloadStart, WeaponState);
		}
	}


	// If we can't shoot, leave
	{
		const auto bHasAmmoReady = bUseClip ? WeaponState.AmmoInClip > 0 : WeaponState.AmmoInPool > 0;
		const auto bCanShoot = bHasAmmoReady && bReceiverCanCycle;
		if (!bCanShoot)
		{
			return false;
		}
	}


	LogMsgWithRole("TickFiring: BANG!");


	// Subtract some ammo
	{
		if (bUseClip)
			--WeaponState.AmmoInClip;
		else
			--WeaponState.AmmoInPool;
	}


	// Fire the shot(s)!
	{
		WeaponState.HasFired = true;
		auto ShotPattern = CalcShotPattern();
		for (auto Direction : ShotPattern)
		{
			Delegate->SpawnAProjectile(Direction);
		}
	}


	// Start the busy timer
	{
		auto DoFireEnd = [&]
		{
			bIsBusy = false;
			GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);
		};

		bIsBusy = true;
		GetWorld()->GetTimerManager().SetTimer(BusyTimerHandle, DoFireEnd, 1.f / ShotsPerSecond, false);
	}


	// Notify changes
	{
		Delegate->ShotFired();

		if (bUseClip)
			Delegate->AmmoInClipChanged(WeaponState.AmmoInClip);
		else
			Delegate->AmmoInPoolChanged(WeaponState.AmmoInPool);
	}

	return false;
}

bool UWeaponReceiverComponent::TickReloading(float DT)
{
	//LogMsgWithRole("EWeaponModes::Reloading");


	// Holster?
	if (InputState.HolsterRequested)
	{
		InputState.HolsterRequested = false;
		GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);
		return ChangeState(EWeaponCommands::UnEquip, WeaponState);
	}

	// Update reload progress
	if (bIsMidReload)
	{
		const auto ElapsedReloadTime = (FDateTime::Now() - ReloadStartTime).GetTotalSeconds();
		WeaponState.ReloadProgress = ElapsedReloadTime / ReloadTime;
		auto str = FString::Printf(TEXT("InProgress %f"), WeaponState.ReloadProgress);
		LogMsgWithRole(str);
	}

	if (bIsBusy) 
	{
		return false; // Do nothing
	}

	// Can't actually reload, just leave.
	if (!CanReload())
	{
		return ChangeState(EWeaponCommands::ReloadEnd, WeaponState);
	}

	// Start Reloading!
	bIsMidReload = true;
	WeaponState.ReloadProgress = 0;
	ReloadStartTime = FDateTime::Now();
	WeaponState.IsAdsing = false;

	bIsBusy = true;
	GetWorld()->GetTimerManager().SetTimer(BusyTimerHandle, this, &UWeaponReceiverComponent::ReloadEnd, ReloadTime, false);

	return false;
}

void UWeaponReceiverComponent::ReloadEnd()
{
	if (WeaponState.Mode != EWeaponModes::Reloading)
	{
		LogMsgWithRole("ReloadEnd() - early out - this shouldn't have run!!");
		return;//HACK HACK HACK. I can't stop this fucking timer for some reason...
	}

	LogMsgWithRole("ReloadEnd()");
	bIsMidReload = false;
	WeaponState.ReloadProgress = 100;

	bIsBusy = false;
	GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);

	// Take ammo from pool
	const int AmmoNeeded = ClipSize - WeaponState.AmmoInClip;
	const int AmmoReceived = (AmmoNeeded > WeaponState.AmmoInPool) ? WeaponState.AmmoInPool : AmmoNeeded;
	WeaponState.AmmoInPool -= AmmoReceived;
	WeaponState.AmmoInClip += AmmoReceived;

	ChangeState(EWeaponCommands::ReloadEnd, WeaponState);
}

bool UWeaponReceiverComponent::ChangeState(EWeaponCommands Cmd, const FWeaponState& InState)
{
	FWeaponState OutState = InState.Clone();

	if (Cmd == EWeaponCommands::FireStart) {
		OutState.Mode = EWeaponModes::Firing;
	}

	if (Cmd == EWeaponCommands::FireEnd) {
		OutState.Mode = EWeaponModes::Idle;
	}

	if (Cmd == EWeaponCommands::ReloadStart) {
		OutState.Mode = EWeaponModes::Reloading;
	}

	if (Cmd == EWeaponCommands::ReloadEnd) {
		OutState.Mode = EWeaponModes::Idle;
	}

	if (Cmd == EWeaponCommands::UnEquip) {
		OutState.Mode = EWeaponModes::UnEquipped;
	}

	if (Cmd == EWeaponCommands::Equip) {
		OutState.Mode = EWeaponModes::Idle;
	}

	LogMsgWithRole(FString::Printf(TEXT("WeapReceiver::ChangeState(%s > %s > %s)"),
		*EWeaponModesStr(InState.Mode), *EWeaponCommandsStr(Cmd), *EWeaponModesStr(OutState.Mode)));

	const bool bWeChangedStates = InState.Mode != OutState.Mode;
	if (bWeChangedStates)
	{
		DoTransitionAction(InState.Mode, OutState.Mode);
	}

	WeaponState = OutState;

	return bWeChangedStates;
}

void UWeaponReceiverComponent::DoTransitionAction(const EWeaponModes OldMode, const EWeaponModes NewMode)
{
	// UnEquipped > Idle
	if (OldMode == EWeaponModes::UnEquipped && NewMode == EWeaponModes::Idle)
	{
		// Stop any actions - should never be true.. TODO Convert these to asserts to make sure we've good elsewhere
		bIsBusy = false;
		GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);

		// Remove all input
		InputState.Reset();

		// Forget all unimportant state
		bIsMidReload = false;
		WeaponState.ReloadProgress = false;
		WeaponState.IsAdsing = false;
		WeaponState.HasFired = false;
	}


	// Any > UnEquipped
	if (NewMode == EWeaponModes::UnEquipped)
	{
		// NEW HERE

		bIsBusy = false;
		GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);
		InputState.Reset();

		// Leave Reload alone! We might want to resume it
		WeaponState.IsAdsing = false;
		WeaponState.HasFired = false;

		LogMsgWithRole("NewMode == Paused");
		LogMsgWithRole(WeaponState.ToString());
	}
}



// Helpers

bool UWeaponReceiverComponent::CanReload() const
{
	return bUseClip &&
		WeaponState.AmmoInClip < ClipSize &&
		WeaponState.AmmoInPool > 0;
}
bool UWeaponReceiverComponent::NeedsReload() const
{
	return bUseClip && WeaponState.AmmoInClip < 1;
}

TArray<FVector> UWeaponReceiverComponent::CalcShotPattern() const
{
	TArray<FVector> Shots;

	const float BarrelAngle = Delegate->GetBarrelDirection().HeadingAngle();
	const float SpreadInRadians = FMath::DegreesToRadians(WeaponState.IsAdsing ? AdsSpread :
		HipfireSpread);

	if (bEvenSpread && ProjectilesPerShot > 1)
	{
		// Shoot projectiles in an even fan with optional shot clumping.
		for (int i = 0; i < ProjectilesPerShot; ++i)
		{
			// TODO factor spread clumping into the base angle and offset per projectile
			// Currently the projectile will spawn out of range of the max spread.

			const float BaseAngle = BarrelAngle - (SpreadInRadians / 2);
			const float OffsetPerProjectile = SpreadInRadians / (ProjectilesPerShot - 1);
			float OffsetHeadingAngle = BaseAngle + i * OffsetPerProjectile;

			// Optionally clump shots together within the fan for natural variance
			if (bSpreadClumping)
			{
				OffsetHeadingAngle += FMath::RandRange(-OffsetPerProjectile / 2, OffsetPerProjectile / 2);
			}

			const FVector ShootDirectionWithSpread = FVector{
				FMath::Cos(OffsetHeadingAngle),
				FMath::Sin(OffsetHeadingAngle), 0 };

			Shots.Add(ShootDirectionWithSpread);
		}
	}
	else
	{
		for (int i = 0; i < ProjectilesPerShot; ++i)
		{
			const float OffsetAngle = FMath::RandRange(-SpreadInRadians / 2, SpreadInRadians / 2);
			const float OffsetHeadingAngle = BarrelAngle + OffsetAngle;

			const FVector ShootDirectionWithSpread = FVector{
				FMath::Cos(OffsetHeadingAngle),
				FMath::Sin(OffsetHeadingAngle), 0 };

			Shots.Add(ShootDirectionWithSpread);
		}
	}


	return Shots;
}

void UWeaponReceiverComponent::DrawAdsLine(const FColor& Color, float LineLength) const
{
	const FVector Start = Delegate->GetBarrelLocation();
	FVector End = Start + Delegate->GetBarrelDirection() * LineLength;

	// Trace line to first hit for end
	FHitResult HitResult;
	const bool bIsHit = GetWorld()->LineTraceSingleByChannel(
		OUT HitResult,
		Start,
		End,
		ECC_Visibility,
		FCollisionQueryParams{ FName(""), false, Delegate->GetOwningPawn()}
	);

	if (bIsHit) End = HitResult.ImpactPoint;

	DrawDebugLine(GetWorld(), Start, End, Color, false, -1., 0, 2.f);
}

void UWeaponReceiverComponent::LogMsgWithRole(FString message)
{
	FString m = GetRoleText() + " " + Delegate->GetWeaponName() + ": " + message;
	UE_LOG(LogTemp, Warning, TEXT("%s"), *m);
}
FString UWeaponReceiverComponent::GetEnumText(ENetRole role)
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
	default:
		return "ERROR";
	}
}

AActor* UWeaponReceiverComponent::GetGrandpappy() const
{
	const auto OwnerOfComponent = GetOwner();
	if (!OwnerOfComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("OwnerOfComponent is null!"));
		return nullptr;
	}
	const auto GrandOwnerOfComponent = OwnerOfComponent->GetOwner();
	if (!GrandOwnerOfComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("GrandOwnerOfComponent is null!"));
		return nullptr;
	}
	return GrandOwnerOfComponent;
}
ENetRole UWeaponReceiverComponent::GetOwnerOwnerLocalRole() const
{
	const auto GrandOwnerOfComponent = GetGrandpappy();
	if (GrandOwnerOfComponent) return GrandOwnerOfComponent->GetLocalRole();
	UE_LOG(LogTemp, Error, TEXT("GrandOwnerOfComponent is null!")); 
	return ENetRole::ROLE_None;
}
ENetRole UWeaponReceiverComponent::GetOwnerOwnerRemoteRole() const
{
	const auto GrandOwnerOfComponent = GetGrandpappy();
	if (GrandOwnerOfComponent) return GrandOwnerOfComponent->GetRemoteRole();
	UE_LOG(LogTemp, Error, TEXT("GrandOwnerOfComponent is null!"));
	return ENetRole::ROLE_None;
}
FString UWeaponReceiverComponent::GetRoleText()
{
	return GetEnumText(GetOwnerOwnerLocalRole()) + " " + GetEnumText(GetOwnerOwnerRemoteRole());
}
