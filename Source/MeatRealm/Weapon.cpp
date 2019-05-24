// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Engine/World.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"

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

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	bCanAction = true;
	AmmoInClip = ClipSize;
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsReloading)
	{
		const FTimespan ElapsedTime = FDateTime::Now() - ReloadStartTime;
		ReloadProgress = ElapsedTime.GetTotalSeconds() / ReloadTime;
		//auto Text = "Reloading";
		//DrawDebugString(GetWorld(), FVector(0, 0, 200), "Reloading", this, FColor::Blue, DeltaTime * .8f);
	}
	/*else if (NeedsReload())
	{
		auto Text = "Empty!";
		DrawDebugString(GetWorld(), FVector(0, 0, 200), "Empty!", this, FColor::Red, DeltaTime * .8f);
	}*/

	if (!bCanAction) return;



	// Reload!
	if (bReloadQueued)
	{
		ClientReloadStart();
		return;
	}


	// Fire!

	// Behaviour: Holding the trigger on an auto gun will auto reload then auto resume firing. Whereas a semiauto requires a new trigger pull to reload and then a new trigger pull to fire again.
	const auto bWeaponCanCycle = bFullAuto || !bHasActionedThisTriggerPull;
	if (bTriggerPulled && bWeaponCanCycle)
	{
		if (NeedsReload())
		{
			ClientReloadStart();
		}
		else
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
	AmmoInClip = ClipSize;
	bReloadQueued = false;

	if (CanActionTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CanActionTimerHandle);
	}
}

bool AWeapon::CanReload() const
{
	return bUseClip && AmmoInClip < ClipSize;
}

bool AWeapon::NeedsReload() const
{
	return bUseClip && AmmoInClip < 1;
}

void AWeapon::Shoot()
{
	//LogMsgWithRole("Shoot");

	if (ProjectileClass == nullptr) return;
	// TODO Set an error message in log suggesting the designer set a projectile class

	UWorld * World = GetWorld();
	if (World == nullptr) return;


	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Instigator;

	// Spawn the projectile at the muzzle.
	AProjectile * Projectile = GetWorld()->SpawnActorAbsolute<AProjectile>(
		ProjectileClass,
		MuzzleLocationComp->GetComponentTransform(), SpawnParams);

	// Set the projectile velocity
	if (Projectile == nullptr) return;

	const auto AdditionalVelocity = GetOwner()->GetVelocity(); // inherits player's velocity
	const auto ShootDirection = MuzzleLocationComp->GetForwardVector();

	Projectile->FireInDirection(ShootDirection, FVector::ZeroVector);
	//UE_LOG(LogTemp, Warning, TEXT("Fired!"));
}



/// INPUT

void AWeapon::Input_PullTrigger()
{
	//LogMsgWithRole("PullTrigger");
	bTriggerPulled = true;
	bHasActionedThisTriggerPull = false;
}

void AWeapon::Input_ReleaseTrigger()
{
	//UE_LOG(LogTemp, Warning, TEXT("ReleaseTrigger!"));
	bTriggerPulled = false;
	bHasActionedThisTriggerPull = false;
}

void AWeapon::Input_Reload()
{
	if (bTriggerPulled || !CanReload()) return;
	//UE_LOG(LogTemp, Warning, TEXT("Input_Reload!"));
	bReloadQueued = true;
}


/// RPC

void AWeapon::RPC_Fire_OnServer_Implementation()
{
	//LogMsgWithRole("RPC_Fire_OnServer_Impl");
	RPC_Fire_RepToClients();
}

bool AWeapon::RPC_Fire_OnServer_Validate()
{
	return true;
}

void AWeapon::RPC_Fire_RepToClients_Implementation()
{
	//LogMsgWithRole("RPC_Fire_RepToClients_Impl");
	Shoot();
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
		return "SimulatedProxy";
	case ROLE_AutonomousProxy:
		return "AutonomouseProxy";
	case ROLE_Authority:
		return "Authority";
	case ROLE_MAX:
	default:
		return "ERROR";
	}
}
FString AWeapon::GetRoleText()
{
	auto Local = GetOwner()->Role;
	auto Remote = GetOwner()->GetRemoteRole();


	if (Remote == ROLE_SimulatedProxy) //&& Local == ROLE_Authority
		return "ListenServer";

	if (Local == ROLE_Authority)
		return "Server";

	if (Local == ROLE_AutonomousProxy) // && Remote == ROLE_Authority
		return "OwningClient";

	if (Local == ROLE_SimulatedProxy) // && Remote == ROLE_Authority
		return "SimClient";

	return "Unknown: " + GetEnumText(Role) + " " + GetEnumText(GetRemoteRole());
}

