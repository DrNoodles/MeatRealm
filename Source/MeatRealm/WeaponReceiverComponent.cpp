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

	if (!HasAuthority()) return;

	WeaponState.AmmoInClip = ClipSizeGiven;
	WeaponState.AmmoInPool = AmmoPoolGiven;
	WeaponState.Mode = EWeaponModes::Ready;
}




// Input state 

void UWeaponReceiverComponent::Input_PullTrigger()
{
	ServerRPC_PullTrigger();
}
void UWeaponReceiverComponent::Input_ReleaseTrigger()
{
	ServerRPC_ReleaseTrigger();
}
void UWeaponReceiverComponent::Input_Reload()
{
	ServerRPC_Reload();
}
void UWeaponReceiverComponent::Input_AdsPressed()
{
	ServerRPC_AdsPressed();
}
void UWeaponReceiverComponent::Input_AdsReleased()
{
	ServerRPC_AdsReleased();
}
bool UWeaponReceiverComponent::TryGiveAmmo()
{
	if (WeaponState.AmmoInPool == AmmoPoolSize) return false;

	WeaponState.AmmoInPool = FMath::Min(WeaponState.AmmoInPool + AmmoGivenPerPickup, AmmoPoolSize);

	return true;
}

// This info can be used to simulate commands onto state in a replayable way!

void UWeaponReceiverComponent::ServerRPC_PullTrigger_Implementation()
{
	InputState.FirePressed = true;
	ShotsFiredThisTriggerPull = 0;
	/*bTriggerPulled = true;
	bHasActionedThisTriggerPull = false;*/
}
bool UWeaponReceiverComponent::ServerRPC_PullTrigger_Validate()
{
	return true;
}

void UWeaponReceiverComponent::ServerRPC_ReleaseTrigger_Implementation()
{
	InputState.FirePressed = false;
	ShotsFiredThisTriggerPull = 0;

	//bTriggerPulled = false;
	//bHasActionedThisTriggerPull = false;
}
bool UWeaponReceiverComponent::ServerRPC_ReleaseTrigger_Validate()
{
	return true;
}

void UWeaponReceiverComponent::ServerRPC_Reload_Implementation()
{
	InputState.ReloadRequested = true;
	/*if (bTriggerPulled || !CanReload()) return;
	bReloadQueued = true;*/
}
bool UWeaponReceiverComponent::ServerRPC_Reload_Validate()
{
	return true;
}

void UWeaponReceiverComponent::ServerRPC_AdsPressed_Implementation()
{
	//LogMsgWithRole("UWeaponReceiverComponent::ServerRPC_AdsPressed_Implementation()");
	InputState.AdsPressed = true;
	//bAdsPressed = true;
}
bool UWeaponReceiverComponent::ServerRPC_AdsPressed_Validate()
{
	return true;
}

void UWeaponReceiverComponent::ServerRPC_AdsReleased_Implementation()
{
	InputState.AdsPressed = false;

	//LogMsgWithRole("UWeaponReceiverComponent::ServerRPC_AdsReleased_Implementation()");
	//bAdsPressed = false;
}
bool UWeaponReceiverComponent::ServerRPC_AdsReleased_Validate()
{
	return true;
}

void UWeaponReceiverComponent::Pause()
{
	check(HasAuthority());
	//LogMsgWithRole(FString::Printf(TEXT("UWeaponReceiverComponent::Holster() %s"), *WeaponName));
	InputState.HolsterRequested = true;
}


void UWeaponReceiverComponent::Resume()
{
	check(HasAuthority());
	//LogMsgWithRole(FString::Printf(TEXT("UWeaponReceiverComponent::Draw() %s"), *WeaponName));
	
	InputState.DrawRequested = true;
	return;


	// NEW HERE

	if (BusyTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);
	}
	bIsBusy = false;

	InputState = FWeaponInputState{};
	WeaponState.ReloadProgress = false;
	WeaponState.HasFired = false;
	WeaponState.Mode = EWeaponModes::Ready;
	// TODO Enable ticking (if disabled on holster)?



	// OLD BELOW


	//// Clear any timer
	//if (CurrentActionTimerHandle.IsValid())
	//{
	//	GetWorld()->GetTimerManager().ClearTimer(CurrentActionTimerHandle);
	//}

	//// Reset state to defaults

	//
	//bHolsterQueued = false;
	//bAdsPressed = false;
	//bTriggerPulled = false;
	//bHasActionedThisTriggerPull = false;

	//// Queue reload if it was mid reload on holster
	//bReloadQueued = bWasReloadingOnHolster;
	//bWasReloadingOnHolster = false;

	//// Ready to roll!
	//bCanAction = true;

}






void UWeaponReceiverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	if (HasAuthority())
	{
		// TODO These ticks might only do 1 operation per tick. Maybe return a bool from each TickFunction if a state was changed so we can reprocess it right away?

		switch (WeaponState.Mode)
		{
		case EWeaponModes::Ready:
			TickReady(DeltaTime);
			break;

		case EWeaponModes::Firing:
			TickFiring(DeltaTime);
			break;

		case EWeaponModes::Reloading:
			TickReloading(DeltaTime);
			break;

		case EWeaponModes::Paused:
			TickPaused(DeltaTime); 
			break;

		case EWeaponModes::None:
		case EWeaponModes::ReloadingPaused:
		default:
			LogMsgWithRole(FString::Printf(TEXT("TickComponent() - WeaponMode unimplemented %d"), WeaponState.Mode));
		}
	}
	else
	{
		// Draw ADS line for self or others
		if (WeaponState.IsAdsing)
		{
			const auto Color = GetOwnerOwnerLocalRole() == ROLE_AutonomousProxy ? AdsLineColor : EnemyAdsLineColor;
			const auto Length = GetOwnerOwnerLocalRole() == ROLE_AutonomousProxy ? AdsLineLength : EnemyAdsLineLength;
			DrawAdsLine(Color, Length);
		}
	}
}


void UWeaponReceiverComponent::TickReady(float DT)
{
	//LogMsgWithRole("EWeaponModes::Ready");

	// Allow fluid enter/exit of ADS state
	WeaponState.IsAdsing = InputState.AdsPressed;

	if (InputState.HolsterRequested)
	{
		InputState.HolsterRequested = false;
		WeaponState = ChangeState(EWeaponCommands::HolsterWeapon, WeaponState);
		return;
	}

	// Ready > Firing
	if (InputState.FirePressed)
	{
		WeaponState = ChangeState(EWeaponCommands::FireStart, WeaponState);
		return;
	}


	// Ready > Reloading
	if (InputState.ReloadRequested)
	{
		// Only process for one frame as we dont have a release
		InputState.ReloadRequested = false;

		if (CanReload())
		{
			WeaponState = ChangeState(EWeaponCommands::ReloadStart, WeaponState);
			return;
		}
	}

}

void UWeaponReceiverComponent::TickPaused(float DeltaTime)
{
	LogMsgWithRole("EWeaponModes::TickPaused");


	// Do a whole lot of not much!

	if (InputState.DrawRequested)
	{
		InputState.DrawRequested = false;
		WeaponState = ChangeState(EWeaponCommands::DrawWeapon, WeaponState);
	}
}

void UWeaponReceiverComponent::TickFiring(float DT)
{
	LogMsgWithRole("EWeaponModes::Firing");


	// Allow fluid enter/exit of ADS state
	WeaponState.IsAdsing = InputState.AdsPressed;


	// If busy, do nothing

	if (bIsBusy) return;






	// Process Input

	if (!InputState.FirePressed)
	{  // Firing > Ready
		WeaponState = ChangeState(EWeaponCommands::FireEnd, WeaponState);

	//	ShotsFired = 0;
		return;
	}






	// Can we fire/reload or do nothing

	const auto bReceiverCanCycle = bFullAuto || ShotsFiredThisTriggerPull == 0;
	if (bReceiverCanCycle && NeedsReload() && CanReload())
	{
		// Reload instead
		WeaponState = ChangeState(EWeaponCommands::ReloadStart, WeaponState);
		return;
	}

	const auto bHasAmmoReady = bUseClip ? WeaponState.AmmoInClip > 0 : WeaponState.AmmoInPool > 0;
	const auto bCanShoot = bHasAmmoReady && bReceiverCanCycle;
	if (!bCanShoot)
	{
		return;
	}


	// Subtract some ammo

	if (bUseClip)
		--WeaponState.AmmoInClip;
	else
		--WeaponState.AmmoInPool;


	// Fire the shot(s)!

	ShotsFiredThisTriggerPull++;
	SpawnProjectiles();


	// Set busy timer

	auto DoFireEnd = [&]
	{
		bIsBusy = false;
		if (BusyTimerHandle.IsValid()) GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);
	};

	bIsBusy = true;
	GetWorld()->GetTimerManager().SetTimer(BusyTimerHandle, DoFireEnd, 1.f / ShotsPerSecond, false);


	// Notify changes

	Delegate->ShotFired();

	if (bUseClip)
		Delegate->AmmoInClipChanged(WeaponState.AmmoInClip);
	else
		Delegate->AmmoInPoolChanged(WeaponState.AmmoInPool);
}

void UWeaponReceiverComponent::TickReloading(float DT)
{
	LogMsgWithRole("EWeaponModes::Reloading");


	// Track reload progress

	if (InputState.HolsterRequested)
	{
		InputState.HolsterRequested = false;
		WeaponState = ChangeState(EWeaponCommands::HolsterWeapon, WeaponState);
		return;
	}

	if (bIsMidReload)
	{
		const auto ElapsedReloadTime = (FDateTime::Now() - ReloadStartTime).GetTotalSeconds();
		WeaponState.ReloadProgress = ElapsedReloadTime / ReloadTime;
		UE_LOG(LogTemp, Warning, TEXT("InProgress %f"), WeaponState.ReloadProgress);
	}


	// If busy, do nothing

	if (bIsBusy) return;



	if (!CanReload())
	{
		WeaponState = ChangeState(EWeaponCommands::ReloadEnd, WeaponState);
		return;
	}





	// Start Reloading!

	bIsMidReload = true;
	WeaponState.ReloadProgress = 0;
	ReloadStartTime = FDateTime::Now();
	WeaponState.IsAdsing = false;


	// Set busy timer

	auto DoReloadEnd = [&]
	{
		bIsMidReload = false;
		WeaponState.ReloadProgress = 100;

		bIsBusy = false;
		if (BusyTimerHandle.IsValid()) GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);

		// Take ammo from pool
		const int AmmoNeeded = ClipSize - WeaponState.AmmoInClip;
		const int AmmoReceived = (AmmoNeeded > WeaponState.AmmoInPool) ? WeaponState.AmmoInPool : AmmoNeeded;
		WeaponState.AmmoInPool -= AmmoReceived;
		WeaponState.AmmoInClip += AmmoReceived;

		WeaponState = ChangeState(EWeaponCommands::ReloadEnd, WeaponState);
	};


	bIsBusy = true;
	GetWorld()->GetTimerManager().SetTimer(BusyTimerHandle, DoReloadEnd, ReloadTime, false);
}


FWeaponState UWeaponReceiverComponent::ChangeState(EWeaponCommands Cmd, const FWeaponState& InState)
{
	/*LogMsgWithRole(FString::Printf(TEXT("UWeaponReceiverComponent::ChangeState(In:%s Cmd:%s)"), 
		*EWeaponModesStr(InState.Mode), *EWeaponCommandsStr(Cmd)));*/

	FWeaponState OutState = InState.Clone();

	if (Cmd == EWeaponCommands::FireStart) {
		ensure(InState.Mode == EWeaponModes::Ready);
		OutState.Mode = EWeaponModes::Firing;
		LastCommand = Cmd;
	}

	if (Cmd == EWeaponCommands::FireEnd) {
		ensure(InState.Mode == EWeaponModes::Firing);
		OutState.Mode = EWeaponModes::Ready;
		LastCommand = Cmd;
	}

	if (Cmd == EWeaponCommands::ReloadStart) {
		ensure(InState.Mode == EWeaponModes::Ready|| InState.Mode == EWeaponModes::Firing);
		OutState.Mode = EWeaponModes::Reloading;
		LastCommand = Cmd;
	}

	if (Cmd == EWeaponCommands::ReloadEnd) {
		ensure(InState.Mode == EWeaponModes::Reloading);
		OutState.Mode = EWeaponModes::Ready;
		LastCommand = Cmd;
	}

	if (Cmd == EWeaponCommands::HolsterWeapon) {
		ensure(InState.Mode == EWeaponModes::Ready || InState.Mode == EWeaponModes::Reloading);
		OutState.Mode = EWeaponModes::Paused;
		LastCommand = Cmd;
	}

	if (Cmd == EWeaponCommands::DrawWeapon) {
		ensure(InState.Mode == EWeaponModes::Paused);
		OutState.Mode = EWeaponModes::Ready;
		LastCommand = Cmd;
	}

	LogMsgWithRole(FString::Printf(TEXT("WeapReceiver::ChangeState(%s > %s > %s)"),
		*EWeaponModesStr(InState.Mode), *EWeaponCommandsStr(Cmd), *EWeaponModesStr(OutState.Mode)));

	return OutState;
}



void UWeaponReceiverComponent::AuthHolsterStart()
{
	check(HasAuthority())
	//LogMsgWithRole("UWeaponReceiverComponent::AuthHolsterStart()");


	// NEW HERE

	if (BusyTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);
	}


	InputState = FWeaponInputState{};
	WeaponState.ReloadProgress = false;
	WeaponState.HasFired = false;
	WeaponState.Mode = EWeaponModes::None;
	// TODO Disable ticking?



	// OLD BELOW
	

	//// Kill any timer running
	//if (CurrentActionTimerHandle.IsValid())
	//{
	//	GetWorld()->GetTimerManager().ClearTimer(CurrentActionTimerHandle);
	//}

	//// Pause reload
	//bWasReloadingOnHolster = bIsReloading;
	//bIsReloading = false;

	//// Reset state to defaults
	//bHolsterQueued = false;
	//bCanAction = false;
	//bAdsPressed = false;
	//bTriggerPulled = false;
	//bHasActionedThisTriggerPull = false;
}


// TODO Figure out what to do with this shit...
//void UWeaponReceiverComponent::IsReloadingChanged()
//{
//	if (WeaponState.Mode == EWeaponModes::Reloading)
//	{
//		// Init reload
//		ClientReloadStartTime = FDateTime::Now();
//
//		// Update UI
//		WeaponState.ReloadProgress = 0;
//	}
//	else
//	{
//		// Finish reload
//		WeaponState.ReloadProgress = 100;
//	}
//
//	Delegate->InReloadingChanged(WeaponState.Mode == EWeaponModes::Reloading);
//	Delegate->OnReloadProgressChanged(WeaponState.ReloadProgress);
//}

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

void UWeaponReceiverComponent::SpawnProjectiles() const
{
	check(HasAuthority())
	//LogMsgWithRole("Shoot");

	auto ShotPattern = CalcShotPattern();
	for (auto Direction : ShotPattern)
	{
		Delegate->SpawnAProjectile(Direction);
	}
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



// Helpers

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
		FCollisionQueryParams{ FName(""), false, GetOwner()}
	);

	if (bIsHit) End = HitResult.ImpactPoint;

	DrawDebugLine(GetWorld(), Start, End, Color, false, -1., 0, 2.f);
}

void UWeaponReceiverComponent::LogMsgWithRole(FString message)
{
	FString m = GetRoleText() + ": " + message;
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

ENetRole UWeaponReceiverComponent::GetOwnerOwnerLocalRole() const
{
	return GetOwner()->GetOwner()->GetLocalRole();
}
ENetRole UWeaponReceiverComponent::GetOwnerOwnerRemoteRole() const
{
	return GetOwner()->GetOwner()->GetRemoteRole();
}
FString UWeaponReceiverComponent::GetRoleText()
{
	
	return 
	GetEnumText(GetOwnerOwnerLocalRole()) + " " + 
	GetEnumText(GetOwnerOwnerRemoteRole());
}
