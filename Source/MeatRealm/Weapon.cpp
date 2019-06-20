// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Engine/World.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Projectile.h"
#include "GameFramework/GameState.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
	
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetGenerateOverlapEvents(false);
	MeshComp->SetCollisionProfileName(TEXT("NoCollision"));
	MeshComp->CanCharacterStepUpOn = ECB_No;

	MuzzleLocationComp = CreateDefaultSubobject<UArrowComponent>(TEXT("MuzzleLocationComp"));
	MuzzleLocationComp->SetupAttachment(RootComponent);
	MuzzleLocationComp->ArrowColor = FColor{ 255,0,0 };
	MuzzleLocationComp->ArrowSize = 0.2;

	MuzzleLightComp = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLightComp"));
	MuzzleLightComp->SetupAttachment(RootComponent);
	MuzzleLightComp->Intensity = 4000;
	MuzzleLightComp->AttenuationRadius = 200;
	MuzzleLightComp->bUseTemperature = true;
	MuzzleLightComp->Temperature = 3000;
	MuzzleLightComp->SetVisibility(false);

	ReceiverComp = CreateDefaultSubobject<UWeaponReceiverComponent>(TEXT("ReceiverComp"));
	ReceiverComp->SetDelegate(this);
	ReceiverComp->SetIsReplicated(true);
}

void AWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeapon, AmmoInClip);
	DOREPLIFETIME(AWeapon, AmmoInPool);
	DOREPLIFETIME(AWeapon, bIsReloading);
	DOREPLIFETIME(AWeapon, bAdsPressed);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority()) return;

	AmmoInClip = ClipSizeGiven;
	AmmoInPool = AmmoPoolGiven;

	Draw();
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		AuthTick(DeltaTime);
	}
	else
	{
		RemoteTick(DeltaTime);
	}
}
void AWeapon::RemoteTick(float DeltaTime)
{
	check (!HasAuthority())

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
		const auto Color = GetOwner()->GetLocalRole() == ROLE_AutonomousProxy ? AdsLineColor : EnemyAdsLineColor;
		const auto Length = GetOwner()->GetLocalRole() == ROLE_AutonomousProxy ? AdsLineLength : EnemyAdsLineLength;
		DrawAdsLine(Color, Length);
	}
}
void AWeapon::AuthTick(float DeltaTime)
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

void AWeapon::AuthFireStart()
{
	//LogMsgWithRole("ClientFireStart()");
	check(HasAuthority())

	bHasActionedThisTriggerPull = true;
	bCanAction = false;
	if (bUseClip) --AmmoInClip;

	SpawnProjectiles();

	// Notify listeners Fire occured
	MultiRPC_NotifyOnShotFired();

	if (AmmoInClip == 0)
	{
		ClientRPC_NotifyOnAmmoWarning();
	}

	GetWorld()->GetTimerManager().SetTimer(
		CanActionTimerHandle, this, &AWeapon::AuthFireEnd, 1.f / ShotsPerSecond, false, -1);
}
void AWeapon::AuthFireEnd()
{
	//LogMsgWithRole("ClientFireEnd()");
	check(HasAuthority())

	bCanAction = true;

	if (CanActionTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CanActionTimerHandle);
	}
}

void AWeapon::Draw()
{
	check(HasAuthority());
	LogMsgWithRole(FString::Printf(TEXT("AWeapon::Draw() %s"), *WeaponName));

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
void AWeapon::QueueHolster()
{
	check(HasAuthority());
	LogMsgWithRole(FString::Printf(TEXT("AWeapon::Holster() %s"), *WeaponName));
	bHolsterQueued = true;
}


void AWeapon::AuthHolsterStart()
{
	check(HasAuthority())
	LogMsgWithRole("AWeapon::AuthHolsterStart()");


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

void AWeapon::AuthReloadStart()
{
	//LogMsgWithRole("ClientReloadStart()");
	check(HasAuthority())

	if (!bUseClip) return;

	bIsReloading = true;
	bCanAction = false;
	bHasActionedThisTriggerPull = true;

	GetWorld()->GetTimerManager().SetTimer(
		CanActionTimerHandle, this, &AWeapon::AuthReloadEnd, ReloadTime, false, -1);
}
void AWeapon::AuthReloadEnd()
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

void AWeapon::OnRep_IsReloadingChanged()
{
	if (bIsReloading)
	{
		// Init reload
		ClientReloadStartTime = FDateTime::Now();

		// Update UI
		ReloadProgress = 0;
		OnReloadStarted.Broadcast();
	}
	else
	{
		// Finish reload
		ReloadProgress = 100;
		OnReloadEnded.Broadcast();
	}
}

bool AWeapon::CanReload() const
{
	return bUseClip && 
		AmmoInClip < ClipSize && 
		AmmoInPool > 0;
}

bool AWeapon::NeedsReload() const
{
	return bUseClip && AmmoInClip < 1;
}

bool AWeapon::IsMatchInProgress() const
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

void AWeapon::ServerRPC_PullTrigger_Implementation()
{
	if (!IsMatchInProgress()) return;

	bTriggerPulled = true;
	bHasActionedThisTriggerPull = false;
}

bool AWeapon::ServerRPC_PullTrigger_Validate()
{
	return true;
}

void AWeapon::ServerRPC_ReleaseTrigger_Implementation()
{
	bTriggerPulled = false;
	bHasActionedThisTriggerPull = false;
}

bool AWeapon::ServerRPC_ReleaseTrigger_Validate()
{
	return true;
}

void AWeapon::ServerRPC_Reload_Implementation()
{
	if (bTriggerPulled || !CanReload()) return;
	bReloadQueued = true;
}

bool AWeapon::ServerRPC_Reload_Validate()
{
	return true;
}

void AWeapon::ServerRPC_AdsPressed_Implementation()
{
	//LogMsgWithRole("AWeapon::ServerRPC_AdsPressed_Implementation()");
	bAdsPressed = true;
}

bool AWeapon::ServerRPC_AdsPressed_Validate()
{
	return true;
}

void AWeapon::ServerRPC_AdsReleased_Implementation()
{
	//LogMsgWithRole("AWeapon::ServerRPC_AdsReleased_Implementation()");
	bAdsPressed = false;
}

bool AWeapon::ServerRPC_AdsReleased_Validate()
{
	return true;
}

void AWeapon::MultiRPC_NotifyOnShotFired_Implementation()
{
	MuzzleLightComp->SetVisibility(true);

	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, [&] { MuzzleLightComp->SetVisibility(false); }, 0.05, false, -1);

	if (OnShotFired.IsBound()) OnShotFired.Broadcast();
}

void AWeapon::ClientRPC_NotifyOnAmmoWarning_Implementation()
{
	LogMsgWithRole("AWeapon::ClientRPC_NotifyOnAmmoWarning_Implementation()");
	if (OnAmmoWarning.IsBound()) OnAmmoWarning.Broadcast();
}

void AWeapon::SpawnProjectiles() const
{
	check(HasAuthority())
	//LogMsgWithRole("Shoot");

	if (ProjectileClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Set a Projectile Class in your Weapon Blueprint to shoot"));
		return;
	};

	auto ShotPattern = CalcShotPattern();
	for (auto Direction : ShotPattern)
	{
		if (!SpawnAProjectile(Direction))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn projectile in Shoot()"));
			return;
		}
	}
}

TArray<FVector> AWeapon::CalcShotPattern() const
{
	TArray<FVector> Shots;

	const float BarrelAngle = MuzzleLocationComp->GetForwardVector().HeadingAngle();
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

bool AWeapon::SpawnAProjectile(const FVector& Direction) const
{
	UWorld* World = GetWorld();
	if (World == nullptr) { return false; }

	// Spawn the projectile at the muzzle.
	AProjectile* Projectile = World->SpawnActorDeferred<AProjectile>(
		ProjectileClass,
		MuzzleLocationComp->GetComponentTransform(),
		GetOwner(),
		Instigator,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Projectile == nullptr) { return false; }


	// Configure it
	Projectile->SetHeroControllerId(HeroControllerId);


	// Fire it!
	UGameplayStatics::FinishSpawningActor(
		Projectile,
		MuzzleLocationComp->GetComponentTransform());
	
	Projectile->FireInDirection(Direction);
	
	return true;
}


/// INPUT

void AWeapon::Input_PullTrigger()
{
	ServerRPC_PullTrigger();
}

void AWeapon::Input_ReleaseTrigger()
{
	ServerRPC_ReleaseTrigger();
}

void AWeapon::Input_Reload()
{
	ServerRPC_Reload();
}

void AWeapon::Input_AdsPressed()
{
	ServerRPC_AdsPressed();
}

void AWeapon::Input_AdsReleased()
{
	ServerRPC_AdsReleased();
}

bool AWeapon::TryGiveAmmo()
{
	//LogMsgWithRole("AWeapon::TryGiveAmmo()");

	if (AmmoInPool == AmmoPoolSize) return false;

	AmmoInPool = FMath::Min(AmmoInPool + AmmoGivenPerPickup, AmmoPoolSize);
	
	return true;
}




/* IReceiverComponentDelegate */
void AWeapon::AmmoInClipChanged(int AmmoInClip)
{
	LogMsgWithRole(FString::Printf(TEXT("AWeapon::AmmoInClipChanged(%d)"), AmmoInClip));
}
void AWeapon::AmmoInPoolChanged(int AmmoInPool)
{
	LogMsgWithRole(FString::Printf(TEXT("AWeapon::AmmoInPoolChanged(%d)"), AmmoInPool));
}
void AWeapon::InReloadingChanged(bool IsReloading)
{
	LogMsgWithRole(FString::Printf(TEXT("AWeapon::InReloadingChanged(%s)"), IsReloading?"True":"False"));
}
void AWeapon::OnReloadProgressChanged(float ReloadProgress)
{
	LogMsgWithRole(FString::Printf(TEXT("AWeapon::OnReloadProgressChanged(%f)"), ReloadProgress));
}
/* End IReceiverComponentDelegate */

/// RPC

void AWeapon::LogMsgWithRole(FString message)
{
	FString m = GetRoleText() + ": " + message;
	UE_LOG(LogTemp, Warning, TEXT("%s"), *m);
}
FString GetEnumText(ENetRole role)
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
FString AWeapon::GetRoleText()
{
	return GetEnumText(Role) + " " + GetEnumText(GetRemoteRole());
}


void AWeapon::DrawAdsLine(const FColor& Color, float LineLength) const
{
	const FVector Start = MuzzleLocationComp->GetComponentLocation();
	FVector End = Start + MuzzleLocationComp->GetComponentRotation().Vector() * LineLength;

	// Trace line to first hit for end
	FHitResult HitResult;
	const bool bIsHit = GetWorld()->LineTraceSingleByChannel(
		OUT HitResult,
		Start,
		End,
		ECC_Visibility,
		FCollisionQueryParams{ FName(""), false, this }
	);

	if (bIsHit) End = HitResult.ImpactPoint;

	DrawDebugLine(GetWorld(), Start, End, Color, false, -1., 0, 2.f);
}
