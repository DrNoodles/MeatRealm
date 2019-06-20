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



void UWeaponReceiverComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority()) return;

	AmmoInClip = ClipSizeGiven;
	AmmoInPool = AmmoPoolGiven;

	Draw();
}
void UWeaponReceiverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (HasAuthority())
	{
		AuthTick(DeltaTime);
	}
	else
	{
		RemoteTick(DeltaTime);
	}
}



void UWeaponReceiverComponent::RemoteTick(float DeltaTime)
{
	check(!HasAuthority())

		if (bIsReloading)
		{
			const auto ElapsedReloadTime = (FDateTime::Now() - ClientReloadStartTime).GetTotalSeconds();

			// Update UI
			ReloadProgress = ElapsedReloadTime / ReloadTime;
			UE_LOG(LogTemp, Warning, TEXT("InProgress %f"), ReloadProgress);
		}

	// Draw ADS line for self or others
	if (bAdsPressed)
	{
		LogMsgWithRole("bAdsPressed");
		const auto Color = GetOwnerOwnerLocalRole() == ROLE_AutonomousProxy ? AdsLineColor : EnemyAdsLineColor;
		const auto Length = GetOwnerOwnerLocalRole() == ROLE_AutonomousProxy ? AdsLineLength : EnemyAdsLineLength;
		DrawAdsLine(Color, Length);
	}
}
void UWeaponReceiverComponent::AuthTick(float DeltaTime)
{
	check(HasAuthority())

		// If a holster is queued, wait for can action unless the action we're in is a reload
		if (bHolsterQueued && (bCanAction || bIsReloading))
		{
			AuthHolsterStart();
			return;
		}

	if (!bCanAction) return;


	// Reload!
	if (bReloadQueued && CanReload())
	{
		AuthReloadStart();
		return;
	}


	// Fire!

	// Behaviour: Holding the trigger on an auto gun will auto reload then auto resume firing. Whereas a semiauto requires a new trigger pull to reload and then a new trigger pull to fire again.
	const auto bWeaponCanCycle = bFullAuto || !bHasActionedThisTriggerPull;
	if (bTriggerPulled && bWeaponCanCycle)
	{
		if (NeedsReload() && CanReload())
		{
			AuthReloadStart();
		}
		else if (AmmoInClip > 0)
		{
			AuthFireStart();
		}
	}
}

void UWeaponReceiverComponent::AuthFireStart()
{
	//LogMsgWithRole("ClientFireStart()");
	check(HasAuthority())

		bHasActionedThisTriggerPull = true;
	bCanAction = false;
	if (bUseClip) --AmmoInClip;

	SpawnProjectiles();

	Delegate->ShotFired();
	Delegate->AmmoInClipChanged(AmmoInClip);

	GetWorld()->GetTimerManager().SetTimer(
		CanActionTimerHandle, this, &UWeaponReceiverComponent::AuthFireEnd, 1.f / ShotsPerSecond, false, -1);
}
void UWeaponReceiverComponent::AuthFireEnd()
{
	//LogMsgWithRole("ClientFireEnd()");
	check(HasAuthority())

		bCanAction = true;

	if (CanActionTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CanActionTimerHandle);
	}
}

void UWeaponReceiverComponent::Draw()
{
	check(HasAuthority());
	//LogMsgWithRole(FString::Printf(TEXT("UWeaponReceiverComponent::Draw() %s"), *WeaponName));

	// Clear any timer
	if (CanActionTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CanActionTimerHandle);
	}

	// Reset state to defaults
	bHolsterQueued = false;
	bAdsPressed = false;
	bTriggerPulled = false;
	bHasActionedThisTriggerPull = false;

	// Queue reload if it was mid reload on holster
	bReloadQueued = bWasReloadingOnHolster;
	bWasReloadingOnHolster = false;

	// Ready to roll!
	bCanAction = true;
}
void UWeaponReceiverComponent::QueueHolster()
{
	check(HasAuthority());
	//LogMsgWithRole(FString::Printf(TEXT("UWeaponReceiverComponent::Holster() %s"), *WeaponName));
	bHolsterQueued = true;
}


void UWeaponReceiverComponent::AuthHolsterStart()
{
	check(HasAuthority())
		//LogMsgWithRole("UWeaponReceiverComponent::AuthHolsterStart()");


	// Kill any timer running
	if (CanActionTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CanActionTimerHandle);
	}

	// Pause reload
	bWasReloadingOnHolster = bIsReloading;
	bIsReloading = false;

	// Reset state to defaults
	bHolsterQueued = false;
	bCanAction = false;
	bAdsPressed = false;
	bTriggerPulled = false;
	bHasActionedThisTriggerPull = false;
}

void UWeaponReceiverComponent::AuthReloadStart()
{
	//LogMsgWithRole("ClientReloadStart()");
	check(HasAuthority())

		if (!bUseClip) return;

	bIsReloading = true;
	bCanAction = false;
	bHasActionedThisTriggerPull = true;

	GetWorld()->GetTimerManager().SetTimer(
		CanActionTimerHandle, this, &UWeaponReceiverComponent::AuthReloadEnd, ReloadTime, false, -1);
}
void UWeaponReceiverComponent::AuthReloadEnd()
{
	//LogMsgWithRole("ClientReloadEnd()");
	check(HasAuthority())

		bIsReloading = false;
	bCanAction = true;
	bReloadQueued = false;

	// Take ammo from pool
	const int AmmoNeeded = ClipSize - AmmoInClip;
	const int AmmoReceived = (AmmoNeeded > AmmoInPool) ? AmmoInPool : AmmoNeeded;
	AmmoInPool -= AmmoReceived;
	AmmoInClip += AmmoReceived;

	if (CanActionTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CanActionTimerHandle);
	}
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

bool UWeaponReceiverComponent::IsMatchInProgress() const
{
	auto World = GetWorld();
	if (World)
	{
		auto GM = World->GetAuthGameMode();
		if (GM)
		{
			auto GS = GM->GetGameState<AGameState>();
			if (GS && GS->IsMatchInProgress())
			{
				return true;
			}
		}
	}
	return false;
}

void UWeaponReceiverComponent::ServerRPC_PullTrigger_Implementation()
{
	if (!IsMatchInProgress()) return;

	bTriggerPulled = true;
	bHasActionedThisTriggerPull = false;
}

bool UWeaponReceiverComponent::ServerRPC_PullTrigger_Validate()
{
	return true;
}

void UWeaponReceiverComponent::ServerRPC_ReleaseTrigger_Implementation()
{
	bTriggerPulled = false;
	bHasActionedThisTriggerPull = false;
}

bool UWeaponReceiverComponent::ServerRPC_ReleaseTrigger_Validate()
{
	return true;
}

void UWeaponReceiverComponent::ServerRPC_Reload_Implementation()
{
	if (bTriggerPulled || !CanReload()) return;
	bReloadQueued = true;
}

bool UWeaponReceiverComponent::ServerRPC_Reload_Validate()
{
	return true;
}

void UWeaponReceiverComponent::ServerRPC_AdsPressed_Implementation()
{
	//LogMsgWithRole("UWeaponReceiverComponent::ServerRPC_AdsPressed_Implementation()");
	bAdsPressed = true;
}

bool UWeaponReceiverComponent::ServerRPC_AdsPressed_Validate()
{
	return true;
}

void UWeaponReceiverComponent::ServerRPC_AdsReleased_Implementation()
{
	//LogMsgWithRole("UWeaponReceiverComponent::ServerRPC_AdsReleased_Implementation()");
	bAdsPressed = false;
}

bool UWeaponReceiverComponent::ServerRPC_AdsReleased_Validate()
{
	return true;
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
	const float SpreadInRadians = FMath::DegreesToRadians(bAdsPressed ? AdsSpread :
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
