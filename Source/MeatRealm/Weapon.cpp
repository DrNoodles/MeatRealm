// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Engine/World.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"

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
}

void AWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeapon, AmmoInClip);
	DOREPLIFETIME(AWeapon, AmmoInPool);
	DOREPLIFETIME(AWeapon, bIsReloading);
	DOREPLIFETIME(AWeapon, ReloadStartTime);
}


void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority()) return;

	bCanAction = true;
	AmmoInClip = ClipSize;
	AmmoInPool = AmmoPoolSize;
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsReloading)
	{
		const auto ElapsedReloadTime = (FDateTime::Now() - ReloadStartTime).GetTotalSeconds();
		ReloadProgress = ElapsedReloadTime / ReloadTime;
	}

	if (!bCanAction) return;



	// Reload!
	if (bReloadQueued && CanReload())
	{
		ClientReloadStart();
		return;
	}


	// Fire!

	// Behaviour: Holding the trigger on an auto gun will auto reload then auto resume firing. Whereas a semiauto requires a new trigger pull to reload and then a new trigger pull to fire again.
	const auto bWeaponCanCycle = bFullAuto || !bHasActionedThisTriggerPull;
	if (bTriggerPulled && bWeaponCanCycle)
	{
		if (NeedsReload() && CanReload())
		{
			ClientReloadStart();
		}
		else if (AmmoInClip > 0)
		{
			ClientFireStart();
		}
	}
}

void AWeapon::ClientFireStart()
{
	//LogMsgWithRole("ClientFireStart()");

	bHasActionedThisTriggerPull = true;
	bCanAction = false;
	if (bUseClip) --AmmoInClip;

	RPC_Fire_OnServer();

	GetWorld()->GetTimerManager().SetTimer(
		CanActionTimerHandle, this, &AWeapon::ClientFireEnd, 1.f / ShotsPerSecond, false, -1);
}
void AWeapon::ClientFireEnd()
{
	bCanAction = true;
	//LogMsgWithRole("ClientFireEnd()");

	if (CanActionTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CanActionTimerHandle);
	}
}

void AWeapon::ClientReloadStart()
{
	if (!bUseClip) return;
	//LogMsgWithRole("ClientReloadStart()");
	ReloadStartTime = FDateTime::Now();
	bIsReloading = true;
	bCanAction = false;
	bHasActionedThisTriggerPull = true;

	GetWorld()->GetTimerManager().SetTimer(
		CanActionTimerHandle, this, &AWeapon::ClientReloadEnd, ReloadTime, false, -1);
}
void AWeapon::ClientReloadEnd()
{
	//LogMsgWithRole("ClientReloadEnd()");
	ReloadProgress = 0;
	bIsReloading = false;
	bCanAction = true;

	// Take ammo from pool
	const int AmmoNeeded = ClipSize - AmmoInClip;
	const int AmmoReceived = (AmmoNeeded > AmmoInPool) ? AmmoInPool : AmmoNeeded;
	AmmoInPool -= AmmoReceived;
	AmmoInClip += AmmoReceived;
	
	bReloadQueued = false;

	if (CanActionTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CanActionTimerHandle);
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

void AWeapon::ServerRPC_PullTrigger_Implementation()
{
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

void AWeapon::Shoot()
{
	//LogMsgWithRole("Shoot");

	if (!HasAuthority()) return;

	if (ProjectileClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Set a Projectile Class in your Weapon Blueprint to shoot"));
		return;
	};

	UWorld * World = GetWorld();
	if (World == nullptr) return;


	// Spawn the projectile at the muzzle.

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Instigator;
	
	AProjectile * Projectile = GetWorld()->SpawnActorAbsolute<AProjectile>(
		ProjectileClass,
		MuzzleLocationComp->GetComponentTransform(), SpawnParams);

	// Set the projectile velocity
	if (Projectile == nullptr) return;

	Projectile->SetHeroControllerId(HeroControllerId);

	

	// Perterb the shot direction by the hipfire spread.
	const auto ShootDirection = MuzzleLocationComp->GetForwardVector();

	const float SpreadInRadians = FMath::DegreesToRadians(HipfireSpread);
	const float OffsetAngle = FMath::RandRange(-SpreadInRadians / 2, SpreadInRadians / 2);
	const float OffsetHeadingAngle = ShootDirection.HeadingAngle() + OffsetAngle;
	const FVector ShootDirectionWithSpread = FVector{ FMath::Cos(OffsetHeadingAngle), FMath::Sin(OffsetHeadingAngle), 0 };

	// Take the shot!
	Projectile->FireInDirection(ShootDirectionWithSpread);

	//UE_LOG(LogTemp, Warning, TEXT("Fired!"));
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

bool AWeapon::TryGiveAmmo()
{
	LogMsgWithRole("AWeapon::TryGiveAmmo()");

	if (AmmoInPool == AmmoPoolSize) return false;

	AmmoInPool = FMath::Min(AmmoInPool + AmmoGivenPerPickup, AmmoPoolSize);
	
	return true;
}


/// RPC

void AWeapon::RPC_Fire_OnServer_Implementation()
{
	//LogMsgWithRole("RPC_Fire_OnServer_Impl");
	Shoot();
}

bool AWeapon::RPC_Fire_OnServer_Validate()
{
	return true;
}


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
	//auto Local = GetOwner()->Role;
	//auto Remote = GetOwner()->GetRemoteRole();


	//if (Remote == ROLE_SimulatedProxy) //&& Local == ROLE_Authority
	//	return "ListenServer";

	//if (Local == ROLE_Authority)
	//	return "Server";

	//if (Local == ROLE_AutonomousProxy) // && Remote == ROLE_Authority
	//	return "OwningClient";

	//if (Local == ROLE_SimulatedProxy) // && Remote == ROLE_Authority
	//	return "SimClient";

	return GetEnumText(Role) + " " + GetEnumText(GetRemoteRole()) + " Ded:" + (IsRunningDedicatedServer() ? "True" : "False");

}

