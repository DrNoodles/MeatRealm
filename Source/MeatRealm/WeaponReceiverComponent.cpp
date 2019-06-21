#include "WeaponReceiverComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "UnrealNetwork.h"
#include "GameFramework/GameState.h"
#include "DrawDebugHelpers.h"

void UWeaponReceiverComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWeaponReceiverComponent, AmmoInClip);
	DOREPLIFETIME(UWeaponReceiverComponent, AmmoInPool);
	DOREPLIFETIME(UWeaponReceiverComponent, bIsReloading);
	DOREPLIFETIME(UWeaponReceiverComponent, bAdsPressed);
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

	AmmoInClip = ClipSizeGiven;
	AmmoInPool = AmmoPoolGiven;

	Draw();
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
	if (AmmoInPool == AmmoPoolSize) return false;

	AmmoInPool = FMath::Min(AmmoInPool + AmmoGivenPerPickup, AmmoPoolSize);

	return true;
}


// TODO Turn these into Commands

// TODO Create an input state that the commands perturb

// This info can be used to simulate commands onto state in a replayable way!

void UWeaponReceiverComponent::ServerRPC_PullTrigger_Implementation()
{
	InputState.FirePressed = true;
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








void UWeaponReceiverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	if (!HasAuthority()) return;

	// TODO 


	//FCommandBase* Command = nullptr;

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

		case EWeaponModes::None:
		case EWeaponModes::Paused:
		case EWeaponModes::ReloadingPaused:
		default: 
			LogMsgWithRole(FString::Printf(TEXT("TickComponent() - WeaponMode unimplemented %d"), WeaponState.Mode));
	}


	//if (HasAuthority())
	//{
	//	AuthTick(DeltaTime);
	//}
	//else
	//{
	//	RemoteTick(DeltaTime);
	//}
}

FWeaponState UWeaponReceiverComponent::ChangeState(EWeaponCommands Cmd, const FWeaponState& InState)
{
	LogMsgWithRole(FString::Printf(TEXT("UWeaponReceiverComponent::ChangeState(Cmd:%d)"), Cmd));
	FWeaponState OutState = InState.Clone();

	switch (Cmd) 
	{ 
		case EWeaponCommands::FireStart:
		{
			ensure(InState.Mode == EWeaponModes::Ready);

			OutState.Mode = EWeaponModes::Firing;
			LastCommand = Cmd;
		}
		break;


		case EWeaponCommands::FireEnd:
		{
			ensure(InState.Mode == EWeaponModes::Firing);

			OutState.Mode = EWeaponModes::Ready;
			LastCommand = Cmd;
		}
		break;


		case EWeaponCommands::ReloadStart:
		{
			ensure(InState.Mode == EWeaponModes::Ready);

			OutState.Mode = EWeaponModes::Reloading;
			LastCommand = Cmd;
		}
		break;


		case EWeaponCommands::ReloadEnd:
		{
			ensure(InState.Mode == EWeaponModes::Reloading);

			OutState.Mode = EWeaponModes::Ready;
			LastCommand = Cmd;
		}
		break;
	}

	return OutState;
}




void UWeaponReceiverComponent::TickReady(float DT)
{
	LogMsgWithRole("EWeaponModes::Ready");


	// Ready > Firing
	if (InputState.FirePressed)
	{
		WeaponState = ChangeState(EWeaponCommands::FireStart, WeaponState);
	}


	// Ready > Reloading
	if (InputState.ReloadRequested)
	{
		// Only process for one frame as we dont have a release
		InputState.ReloadRequested = false;

		if (CanReload())
		{
			WeaponState = ChangeState(EWeaponCommands::ReloadStart, WeaponState);
		}
	}

	// Allow fluid enter/exit of ADS state
	WeaponState.IsAdsing = InputState.AdsPressed;
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

		ShotsFired = 0;
		return;
	}


	// If using clip and it's empty, transition to reload


	// Can we fire?

	const auto bHasAmmoReady = bUseClip ? AmmoInClip > 0 : AmmoInPool > 0;
	const auto bReceiverCanCycle = bFullAuto || ShotsFired == 0;
	const bool bCanShoot = bHasAmmoReady && bReceiverCanCycle;
	if (!bCanShoot)
	{
		return;
	}


	// Subtract some ammo

	if (bUseClip)
		--AmmoInClip;
	else
		--AmmoInPool;


	// Fire the shot(s)!

	ShotsFired++;
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
		Delegate->AmmoInClipChanged(AmmoInClip);
	else
		Delegate->AmmoInPoolChanged(AmmoInPool);
}

void UWeaponReceiverComponent::TickReloading(float DT)
{
	LogMsgWithRole("EWeaponModes::Reloading");


	//// Report reload progress for clients
	//if (!HasAuthority() && bIsReloading)
	//{
	//	const auto ElapsedReloadTime = (FDateTime::Now() - ClientReloadStartTime).GetTotalSeconds();

	//	// Update UI
	//	ReloadProgress = ElapsedReloadTime / ReloadTime;
	//	UE_LOG(LogTemp, Warning, TEXT("InProgress %f"), ReloadProgress);
	//}






	// If busy, do nothing

	if (bIsBusy) return;

	bIsReloading = true;

	// Force Ads off while reloading
	//WeaponState.IsAdsing = false;





	// Set busy timer

	auto DoReloadEnd = [&]
	{
		bIsReloading = false;
		bIsBusy = false;
		if (BusyTimerHandle.IsValid()) GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);

		// Take ammo from pool
		const int AmmoNeeded = ClipSize - AmmoInClip;
		const int AmmoReceived = (AmmoNeeded > AmmoInPool) ? AmmoInPool : AmmoNeeded;
		AmmoInPool -= AmmoReceived;
		AmmoInClip += AmmoReceived;


		WeaponState = ChangeState(EWeaponCommands::ReloadEnd, WeaponState);
	};


	bIsBusy = true;
	GetWorld()->GetTimerManager().SetTimer(BusyTimerHandle, DoReloadEnd, ReloadTime, false);
}





void UWeaponReceiverComponent::RemoteTick(float DeltaTime)
{
	//check(!HasAuthority())

		//if (bIsReloading)
		//{
		//	const auto ElapsedReloadTime = (FDateTime::Now() - ClientReloadStartTime).GetTotalSeconds();

		//	// Update UI
		//	ReloadProgress = ElapsedReloadTime / ReloadTime;
		//	UE_LOG(LogTemp, Warning, TEXT("InProgress %f"), ReloadProgress);
		//}

	//// Draw ADS line for self or others
	//if (bAdsPressed)
	//{
	//	LogMsgWithRole("bAdsPressed");
	//	const auto Color = GetOwnerOwnerLocalRole() == ROLE_AutonomousProxy ? AdsLineColor : EnemyAdsLineColor;
	//	const auto Length = GetOwnerOwnerLocalRole() == ROLE_AutonomousProxy ? AdsLineLength : EnemyAdsLineLength;
	//	DrawAdsLine(Color, Length);
	//}
}
void UWeaponReceiverComponent::AuthTick(float DeltaTime)
{
	//check(HasAuthority())

	//	// If a holster is queued, wait for can action unless the action we're in is a reload
	//	if (bHolsterQueued && (bCanAction || bIsReloading))
	//	{
	//		AuthHolsterStart();
	//		return;
	//	}

	//if (!bCanAction) return;


	//// Reload!
	//if (bReloadQueued && CanReload())
	//{
	//	AuthReloadStart();
	//	return;
	//}


	//// Fire!

	//// Behaviour: Holding the trigger on an auto gun will auto reload then auto resume firing. Whereas a semiauto requires a new trigger pull to reload and then a new trigger pull to fire again.
	//const auto bWeaponCanCycle = bFullAuto || !bHasActionedThisTriggerPull;
	//if (bTriggerPulled && bWeaponCanCycle)
	//{
	//	if (NeedsReload() && CanReload())
	//	{
	//		AuthReloadStart();
	//	}
	//	else if (AmmoInClip > 0)
	//	{
	//		AuthFireStart();
	//	}
	//}
}

void UWeaponReceiverComponent::Draw()
{
	check(HasAuthority());

	//LogMsgWithRole(FString::Printf(TEXT("UWeaponReceiverComponent::Draw() %s"), *WeaponName));




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
void UWeaponReceiverComponent::QueueHolster()
{
	check(HasAuthority());
	//LogMsgWithRole(FString::Printf(TEXT("UWeaponReceiverComponent::Holster() %s"), *WeaponName));
	//bHolsterQueued = true;
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


void UWeaponReceiverComponent::OnRep_IsReloadingChanged()
{
	if (bIsReloading)
	{
		// Init reload
		ClientReloadStartTime = FDateTime::Now();

		// Update UI
		ReloadProgress = 0;
	}
	else
	{
		// Finish reload
		ReloadProgress = 100;
	}

	Delegate->InReloadingChanged(bIsReloading);
	Delegate->OnReloadProgressChanged(ReloadProgress);
}

bool UWeaponReceiverComponent::CanReload() const
{
	return bUseClip &&
		AmmoInClip < ClipSize &&
		AmmoInPool > 0;
}
bool UWeaponReceiverComponent::NeedsReload() const
{
	return bUseClip && AmmoInClip < 1;
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
