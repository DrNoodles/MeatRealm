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

//void UWeaponReceiverComponent::OnRep_WeaponState()
//{
//	LogMsgWithRole("UWeaponReceiverComponent::OnRep_WeaponState()");
//}

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

	WeaponState = ChangeState(EWeaponCommands::DrawWeapon, WeaponState);
}



// Input state 

void UWeaponReceiverComponent::RequestResume()
{
	check(HasAuthority());
	LogMsgWithRole(FString::Printf(TEXT("InputState.DrawRequested = true")));
	InputState.DrawRequested = true;
}
void UWeaponReceiverComponent::RequestPause()
{
	check(HasAuthority());
	LogMsgWithRole(FString::Printf(TEXT("InputState.HolsterRequested = true")));
	InputState.HolsterRequested = true;
}
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
	WeaponState.HasFired = false;
}
void UWeaponReceiverComponent::ServerRPC_ReleaseTrigger_Implementation()
{
	InputState.FirePressed = false;
	WeaponState.HasFired = false;
}
void UWeaponReceiverComponent::ServerRPC_Reload_Implementation()
{
	InputState.ReloadRequested = true;
}
void UWeaponReceiverComponent::ServerRPC_AdsPressed_Implementation()
{
	//LogMsgWithRole("UWeaponReceiverComponent::ServerRPC_AdsPressed_Implementation()");
	InputState.AdsPressed = true;
	WeaponState.IsAdsing = InputState.AdsPressed;

}
void UWeaponReceiverComponent::ServerRPC_AdsReleased_Implementation()
{
	InputState.AdsPressed = false;
	WeaponState.IsAdsing = InputState.AdsPressed;
}


bool UWeaponReceiverComponent::ServerRPC_PullTrigger_Validate() { return true; }
bool UWeaponReceiverComponent::ServerRPC_ReleaseTrigger_Validate(){ return true; }
bool UWeaponReceiverComponent::ServerRPC_Reload_Validate() { return true; }
bool UWeaponReceiverComponent::ServerRPC_AdsPressed_Validate() { return true; }
bool UWeaponReceiverComponent::ServerRPC_AdsReleased_Validate() { return true; }






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
	
		default:
			LogMsgWithRole(FString::Printf(TEXT("TickComponent() - WeaponMode unimplemented %d"), *EWeaponModesStr(WeaponState.Mode)));
		}
	}
	else
	{
		// Draw ADS line for self or others // TODO Remove this from ReceiverComp, back into Weapon
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
	//WeaponState.IsAdsing = InputState.AdsPressed;

	if (InputState.HolsterRequested)
	{
		LogMsgWithRole("EWeaponModes::TickReady - Processing HolsterRequested");

		InputState.HolsterRequested = false;
		WeaponState = ChangeState(EWeaponCommands::HolsterWeapon, WeaponState);
		return;
	}

	// Ready > Firing
	if (InputState.FirePressed)
	{
		LogMsgWithRole("EWeaponModes::TickReady - Processing FirePressed");

		WeaponState = ChangeState(EWeaponCommands::FireStart, WeaponState);
		return;
	}


	// Ready > Reloading
	if (InputState.ReloadRequested)
	{
		LogMsgWithRole("EWeaponModes::TickReady - Processing ReloadRequested");
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
	auto str = FString::Printf(TEXT("EWeaponModes::TickPaused %s"), *WeaponState.ToString());
	LogMsgWithRole(str);

	if (WeaponState.IsAdsing) WeaponState.IsAdsing = false;

	if (InputState.DrawRequested)
	{
		LogMsgWithRole("EWeaponModes::TickPaused - Processed Draw Request");

		InputState.DrawRequested = false;

		if(bIsMidReload)
			WeaponState = ChangeState(EWeaponCommands::ReloadStart, WeaponState);
		else
			WeaponState = ChangeState(EWeaponCommands::DrawWeapon, WeaponState);
	}
}




void UWeaponReceiverComponent::TickFiring(float DT)
{
	//LogMsgWithRole("EWeaponModes::Firing");


	// Allow ADS on/off any timestate


	if (bIsBusy) return;
	const auto bReceiverCanCycle = bFullAuto || !WeaponState.HasFired;


	// Process State Transitions
	{
		if (!InputState.FirePressed)
		{ 
			WeaponState = ChangeState(EWeaponCommands::FireEnd, WeaponState);
			return;
		}
		if (InputState.HolsterRequested)
		{
			WeaponState = ChangeState(EWeaponCommands::HolsterWeapon, WeaponState);
			return;
		}
		if (bReceiverCanCycle && NeedsReload() && CanReload())
		{
			// Reload instead
			WeaponState = ChangeState(EWeaponCommands::ReloadStart, WeaponState);
			return;
		}
	}


	// If we can't shoot, leave
	{
		const auto bHasAmmoReady = bUseClip ? WeaponState.AmmoInClip > 0 : WeaponState.AmmoInPool > 0;
		const auto bCanShoot = bHasAmmoReady && bReceiverCanCycle;
		if (!bCanShoot)
		{
			return;
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
			if (BusyTimerHandle.IsValid()) GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);
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
}



void UWeaponReceiverComponent::TickReloading(float DT)
{
	//LogMsgWithRole("EWeaponModes::Reloading");


	// Holster?
	if (InputState.HolsterRequested)
	{
		InputState.HolsterRequested = false;
		WeaponState = ChangeState(EWeaponCommands::HolsterWeapon, WeaponState);
		return;
	}

	// Update reload progress
	if (bIsMidReload)
	{
		const auto ElapsedReloadTime = (FDateTime::Now() - ReloadStartTime).GetTotalSeconds();
		WeaponState.ReloadProgress = ElapsedReloadTime / ReloadTime;
		UE_LOG(LogTemp, Warning, TEXT("InProgress %f"), WeaponState.ReloadProgress);
	}


	if (bIsBusy) return;


	// Can't actually reload, just leave.
	if (!CanReload())
	{
		WeaponState = ChangeState(EWeaponCommands::ReloadEnd, WeaponState);
		return;
	}


	// Define reload end method
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


	// Start Reloading!
	bIsMidReload = true;
	WeaponState.ReloadProgress = 0;
	ReloadStartTime = FDateTime::Now();
	WeaponState.IsAdsing = false;

	bIsBusy = true;
	GetWorld()->GetTimerManager().SetTimer(BusyTimerHandle, DoReloadEnd, ReloadTime, false);
}


FWeaponState UWeaponReceiverComponent::ChangeState(EWeaponCommands Cmd, const FWeaponState& InState)
{
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
		ensure(InState.Mode == EWeaponModes::Ready || InState.Mode == EWeaponModes::Firing || InState.Mode == EWeaponModes::Paused);
		OutState.Mode = EWeaponModes::Reloading;
		LastCommand = Cmd;
	}

	if (Cmd == EWeaponCommands::ReloadEnd) {
		ensure(InState.Mode == EWeaponModes::Reloading);
		OutState.Mode = EWeaponModes::Ready;
		LastCommand = Cmd;
	}

	if (Cmd == EWeaponCommands::HolsterWeapon) {
		ensure(InState.Mode == EWeaponModes::Ready || InState.Mode == EWeaponModes::Firing || InState.Mode == EWeaponModes::Reloading);
		OutState.Mode = EWeaponModes::Paused;
		LastCommand = Cmd;
	}

	if (Cmd == EWeaponCommands::DrawWeapon) {
		ensure(InState.Mode == EWeaponModes::Paused || InState.Mode == EWeaponModes::None);
			OutState.Mode = EWeaponModes::Ready;
		LastCommand = Cmd;
	}

	LogMsgWithRole(FString::Printf(TEXT("WeapReceiver::ChangeState(%s > %s > %s)"),
		*EWeaponModesStr(InState.Mode), *EWeaponCommandsStr(Cmd), *EWeaponModesStr(OutState.Mode)));

	const bool bWeChangedStates = InState.Mode != OutState.Mode;
	if (bWeChangedStates)
	{
		DoTransitionAction(InState.Mode, OutState.Mode);
	}

	return OutState;
}


void UWeaponReceiverComponent::DoTransitionAction(const EWeaponModes OldMode, const EWeaponModes NewMode)
{
	// None/Paused > Ready
	if ((OldMode == EWeaponModes::None || OldMode == EWeaponModes::Paused) && NewMode == EWeaponModes::Ready)
	{
		// Stop any actions - should never be true.. TODO Convert these to asserts to make sure we've good elsewhere
		bIsBusy = false;
		if (BusyTimerHandle.IsValid()) GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);

		// Remove all input
		InputState = FWeaponInputState{};

		// Forget all unimportant state
		bIsMidReload = false;
		WeaponState.ReloadProgress = false;
		WeaponState.IsAdsing = false;
		WeaponState.HasFired = false;
	}


	// Any > Paused
	if (NewMode == EWeaponModes::Paused)
	{
		// NEW HERE

		bIsBusy = false;
		if (BusyTimerHandle.IsValid()) GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);

		InputState = FWeaponInputState{};

		// Leave Reload alone! We might want to resume it
		WeaponState.IsAdsing = false;
		WeaponState.HasFired = false;

		LogMsgWithRole("NewMode == Paused");
		LogMsgWithRole(WeaponState.ToString());
	}
}






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
