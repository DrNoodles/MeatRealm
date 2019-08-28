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

	if (HasAuthority())
	{
		WeaponState.AmmoInClip = ClipSizeGiven;
		WeaponState.AmmoInPool = AmmoPoolGiven;

		//LogMsgWithRole(FString::Printf(TEXT("BeginPlay - Clip:%d Pool:%d"), WeaponState.AmmoInClip, WeaponState.AmmoInPool));
	}
}
	



// Input state 

void UWeaponReceiverComponent::DrawWeapon()
{
	//LogMsgWithRole(FString::Printf(TEXT("InputState.DrawRequested = true")));
	InputState.DrawRequested = true;
}
void UWeaponReceiverComponent::HolsterWeapon()
{
	//LogMsgWithRole(FString::Printf(TEXT("InputState.HolsterRequested = true")));
	InputState.HolsterRequested = true;
}
void UWeaponReceiverComponent::PullTrigger()
{
	InputState.FireRequested = true;

	WeaponState.BurstCount = 0;
	ShotTimes.Empty();

}
void UWeaponReceiverComponent::ReleaseTrigger()
{
	InputState.FireRequested = false;

	WeaponState.BurstCount = 0;
	ShotTimes.Empty();
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
bool UWeaponReceiverComponent::CanGiveAmmo()
{
	return CanReceiveAmmo && WeaponState.AmmoInPool < AmmoPoolSize;
}
bool UWeaponReceiverComponent::TryGiveAmmo()
{
	if (WeaponState.AmmoInPool == AmmoPoolSize) return false;

	WeaponState.AmmoInPool = FMath::Min(WeaponState.AmmoInPool + AmmoGivenPerPickup, AmmoPoolSize);

	return true;
}

void UWeaponReceiverComponent::CancelAnyReload()
{
	if (WeaponState.Mode==EWeaponModes::Reloading)
	{
		bIsBusy = false;
		GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);
		ChangeState(EWeaponCommands::ReloadEnd, WeaponState);
	}
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
		{
			//auto str = FString::Printf(TEXT("EWeaponModes::TickPaused %s"), *WeaponState.ToString());

			if (InputState.DrawRequested)
			{
				InputState.DrawRequested = false;
				ChangeState(EWeaponCommands::Equip, WeaponState);
			}
		}
		break;

		case EWeaponModes::Equipping:
		{
			/*if (InputState.HolsterRequested)
			{
				InputState.HolsterRequested = false;
				ChangeState(EWeaponCommands::UnEquip, WeaponState);
			}*/
			//ChangeState(EWeaponCommands::Equip, WeaponState);

			break;
		}

		default:
			LogMsgWithRole(FString::Printf(TEXT("TickComponent() - WeaponMode unimplemented %s"), *EWeaponModesStr(WeaponState.Mode)));
		}
	}


	// Draw ADS line for self or others 
	// TODO Remove this from ReceiverComp, back into Weapon
	if (!HasAuthority() && WeaponState.IsAdsing 
		&& (WeaponState.Mode == EWeaponModes::Idle || WeaponState.Mode == EWeaponModes::Firing))
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
		//LogMsgWithRole("EWeaponModes::TickReady - Processing HolsterRequested");
		InputState.HolsterRequested = false;
		return ChangeState(EWeaponCommands::UnEquip, WeaponState);
	}




	// Ready > Firing
	if (InputState.FireRequested)
	{
		//LogMsgWithRole("EWeaponModes::TickReady - Processing FirePressed");
		return ChangeState(EWeaponCommands::FireStart, WeaponState);
	}


	// Ready > Reloading (forced when clip empty)
	//if (NeedsReload() && CanReload())
	//{
	//	// Reload instead
	//	return ChangeState(EWeaponCommands::ReloadStart, WeaponState);
	//}


	// Ready > Reloading
	if (InputState.ReloadRequested)
	{
		//LogMsgWithRole("EWeaponModes::TickReady - Processing ReloadRequested");
		// Only process for one frame as we dont have a release

		InputState.ReloadRequested = false;

		if (CanReload())
		{
			return ChangeState(EWeaponCommands::ReloadStart, WeaponState);
		}
	}

	return false;
}

//bool UWeaponReceiverComponent::TickUnEquipped(float DeltaTime)
//{
//	
//}

bool UWeaponReceiverComponent::TickFiring(float DT)
{
	//LogMsgWithRole("EWeaponModes::Firing");

	if (InputState.HolsterRequested)
	{
		return ChangeState(EWeaponCommands::UnEquip, WeaponState);
	}



	if (bIsBusy) return false;
	const auto bReceiverCanCycle = bFullAuto || WeaponState.BurstCount == 0;


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


	//LogMsgWithRole("TickFiring: BANG!");


	// Subtract some ammo
	{
		if (bUseClip)
			--WeaponState.AmmoInClip;
		else
			--WeaponState.AmmoInPool;
	}


	// Fire the shot(s)!
	auto ShotRate = 1.f / ShotsPerSecond;


	float ShotRateError = 0;
	{

		// Record time of shot and compute how much slack time till the next shot
		float Now = GetWorld()->TimeSeconds;

		if (bFullAuto && ShotTimes.Num() > 0)
		{
			float TimeSinceFirstShot = Now - ShotTimes[0];
			float ExpectedTimeSinceFirstShot = ShotRate * WeaponState.BurstCount;
			float TotalError = TimeSinceFirstShot - ExpectedTimeSinceFirstShot;

			//UE_LOG(LogTemp, Warning, TEXT("TimeSinceFirstShot:%f, ExpectedTimeSinceFirstShot:%f, Err:%f"), TimeSinceFirstShot, ExpectedTimeSinceFirstShot, TotalError);

			ShotRateError = TotalError;
		}

		// Store shot timing/count
		ShotTimes.Add(Now);
		WeaponState.BurstCount++;

		// Shoot the damn thing!
		auto ShotPattern = CalcShotPattern();
		for (auto Direction : ShotPattern)
		{
			Delegate->SpawnAProjectile(Direction);
		}
	}


	// Start the busy timer
	{
		bIsBusy = true;

		float TimeTillNextShot = FMath::Max<float>(ShotRate - ShotRateError, SMALL_NUMBER);
		TimeTillNextShot -= 1/60.f; // TODO Hack! Generally the shots are delayed by about 1 tick @ 60Hz from the requested time. 

		GetWorld()->GetTimerManager().SetTimer(BusyTimerHandle, this, &UWeaponReceiverComponent::FireEnd, TimeTillNextShot, false);
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
void UWeaponReceiverComponent::FireEnd()
{
	//LogMsgWithRole("FireEnd");
	bIsBusy = false;
	GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);
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
		WeaponState.ReloadProgress = ElapsedReloadTime / GetReloadTime();
		//auto str = FString::Printf(TEXT("InProgress %f"), WeaponState.ReloadProgress);
		//LogMsgWithRole(str);
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

	LogMsgWithRole("Starting Reload");

	// Start Reloading!
	bIsMidReload = true;
	WeaponState.ReloadProgress = 0;
	ReloadStartTime = FDateTime::Now();
	WeaponState.IsAdsing = false;

	bIsBusy = true;
	GetWorld()->GetTimerManager().SetTimer(BusyTimerHandle, this, &UWeaponReceiverComponent::ReloadEnd, GetReloadTime(), false);

	return false;
}

void UWeaponReceiverComponent::ReloadEnd()
{
	if (WeaponState.Mode != EWeaponModes::Reloading)
	{
		//LogMsgWithRole("ReloadEnd() - early out - this shouldn't have run!!");
		return;//HACK HACK HACK. I can't stop this fucking timer for some reason...
	}

	//LogMsgWithRole("ReloadEnd()");
	bIsMidReload = false;
	WeaponState.ReloadProgress = 100;

	bIsBusy = false;
	GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);

	// Take ammo from pool
	const int AmmoNeeded = ClipSize - WeaponState.AmmoInClip;
	const int AmmoReceived = (AmmoNeeded > WeaponState.AmmoInPool) ? WeaponState.AmmoInPool : AmmoNeeded;
	WeaponState.AmmoInPool -= AmmoReceived;
	WeaponState.AmmoInClip += AmmoReceived;
	WeaponState.IsAdsing = InputState.AdsRequested;

	ChangeState(EWeaponCommands::ReloadEnd, WeaponState);
}

bool UWeaponReceiverComponent::ChangeState(EWeaponCommands Cmd, FWeaponState& WeapState)
{
	//FWeaponState OutState = InState;

	const auto OldMode = WeapState.Mode;

	if (Cmd == EWeaponCommands::FireStart) {
		WeapState.Mode = EWeaponModes::Firing;
	}

	if (Cmd == EWeaponCommands::FireEnd) {
		WeapState.Mode = EWeaponModes::Idle;
	}

	if (Cmd == EWeaponCommands::ReloadStart) {
		WeapState.Mode = EWeaponModes::Reloading;
	}

	if (Cmd == EWeaponCommands::ReloadEnd) {
		WeapState.Mode = EWeaponModes::Idle;
	}

	if (Cmd == EWeaponCommands::UnEquip) {
		WeapState.Mode = EWeaponModes::UnEquipped;
	}

	if (Cmd == EWeaponCommands::Equip) {
		WeapState.Mode = EWeaponModes::Equipping;
	}

	if (Cmd == EWeaponCommands::EquipEnd) {
		WeapState.Mode = EWeaponModes::Idle;
	}

	LogMsgWithRole(FString::Printf(TEXT("WeapReceiver::ChangeState(%s > %s > %s)"),
		*EWeaponModesStr(OldMode), *EWeaponCommandsStr(Cmd), *EWeaponModesStr(WeapState.Mode)));

	const bool bWeChangedStates = OldMode != WeapState.Mode;
	if (bWeChangedStates)
	{
		DoTransitionAction(OldMode, WeapState.Mode, WeapState);
	}

	//WeaponState = OutState;

	return bWeChangedStates;
}
//void UWeaponReceiverComponent::EquipEnd()
//{
//	//LogMsgWithRole("EquipEnd()");
//	ChangeState(EWeaponCommands::EquipEnd, WeaponState);
//}
void UWeaponReceiverComponent::DoTransitionAction(const EWeaponModes OldMode, const EWeaponModes NewMode, FWeaponState& NewState)
{
	// Any > Equipping
	if (NewMode == EWeaponModes::Equipping)
	{
		//LogMsgWithRole("NewMode == Equipping");

		// Stop any actions - should never be true.. TODO Convert these to asserts to make sure we've good elsewhere
		bIsBusy = false;
		GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);

		// Remove all input
		InputState.Reset();

		// Forget all unimportant state
		NewState.ReloadProgress = 0;
		NewState.IsAdsing = false;
		
		NewState.BurstCount = 0;
		ShotTimes.Empty();

		ChangeState(EWeaponCommands::EquipEnd, WeaponState);

	/*	GetWorld()->GetTimerManager().SetTimer(BusyTimerHandle, this, &UWeaponReceiverComponent::EquipEnd, Delegate->GetDrawDuration(), false);*/
	}


	// Any > UnEquipped
	if (NewMode == EWeaponModes::UnEquipped)
	{
		//LogMsgWithRole("NewMode == UnEquipped");

		// NEW HERE

		bIsBusy = false;
		GetWorld()->GetTimerManager().ClearTimer(BusyTimerHandle);

		InputState.Reset();

		// Leave Reload alone! We might want to resume it
		NewState.ReloadProgress = 0;
		NewState.IsAdsing = false;
		NewState.BurstCount = 0;
		ShotTimes.Empty();

		//LogMsgWithRole(WeaponState.ToString());
	}


	if (OldMode == EWeaponModes::Firing)
	{
		//LogMsgWithRole("OldMode == Firing");


		// Track the fire rate error and report as error if it's above a tolerance

		// Compute the time between shots if we've had a burst of at least 3
		const auto Num = ShotTimes.Num();
		auto TotalDiff = 0.f;
		int Count = 0;
		const int Skip = 1;

		if (bFullAuto && Num > Skip+1)
		{
			for (int i = Skip+1; i < Num; ++i)
			{
				const auto First = ShotTimes[i-1];
				const auto Second = ShotTimes[i];
				const auto Diff = Second - First;
				TotalDiff += Diff;
				++Count;
			}

			// Report
			const float Avg = TotalDiff / Count;
			const float Expected = 1.f / ShotsPerSecond;
			const float AvgDiff = Avg - Expected;
			const float DiffPercentage = (Avg / Expected - 1) * 100;

			//LogMsgWithRole(FString::Printf(TEXT("Avg:%f, Expected:%f, AvgDiff:%f %.3f%%"), Avg, Expected, AvgDiff, DiffPercentage));

			const float ErrorTolerance = 1.5;
			if (FMath::Abs(DiffPercentage) > ErrorTolerance) // error tolerance
			{
				UE_LOG(LogTemp, Error, TEXT("Gun fire rate error is %.3f%%. Time before shot:"), DiffPercentage);
				for (int j = 1; j < ShotTimes.Num(); ++j)
				{
					const auto Diff = ShotTimes[j] - ShotTimes[j - 1];
					UE_LOG(LogTemp, Error, TEXT("   Shot%d: %f"), j+1, Diff);
				}
			}
		}

		ShotTimes.Empty();
		NewState.BurstCount = 0;
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
	const float SpreadInRadians = FMath::DegreesToRadians(WeaponState.IsAdsing ? GetAdsSpread() :
		GetHipfireSpread());

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
	FVector BarrelLocation = Delegate->GetBarrelLocation();
	FVector BarrelDirection = Delegate->GetBarrelDirection();

	const FVector Start = BarrelLocation + BarrelDirection;// *100; // dont draw line for first meter
	FVector End = BarrelLocation + BarrelDirection * LineLength;

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
