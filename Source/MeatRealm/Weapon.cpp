// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Engine/World.h"
#include "Components/ArrowComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Projectile.h"
#include "HeroCharacter.h"
#include "Interfaces/AffectableInterface.h"


AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
	
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;

	SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMeshComp->SetupAttachment(RootComponent);
	SkeletalMeshComp->SetGenerateOverlapEvents(false);
	SkeletalMeshComp->SetCollisionProfileName(TEXT("NoCollision"));
	SkeletalMeshComp->CanCharacterStepUpOn = ECB_No;

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
void AWeapon::ConfigWeapon(FWeaponConfig& Config) const
{
	check(HasAuthority());

	if (Config.AmmoInClip > -1) ReceiverComp->ClipSizeGiven = Config.AmmoInClip;
	if (Config.AmmoInPool > -1) ReceiverComp->AmmoPoolGiven = Config.AmmoInPool;

	//LogMsgWithRole(FString::Printf(TEXT("OverrideAmmoGiven - Clip:%d Pool:%d"), Config.AmmoInClip, Config.AmmoInPool));
}


// INPUT //////////////////////

void AWeapon::Equip()
{
	if (!HasAuthority())
	{
		ServerRPC_Equip();
		return; // TODO Remove return to enable client preditiction (currently broken)
	}
	ReceiverComp->DrawWeapon();
}
void AWeapon::ServerRPC_Equip_Implementation()
{
	Equip();
}
bool AWeapon::ServerRPC_Equip_Validate()
{
	return true;
}

void AWeapon::Unequip()
{
	if (!HasAuthority())
	{
		ServerRPC_Unequip();
		return; // TODO Remove return to enable client preditiction (currently broken)
	}
	ReceiverComp->HolsterWeapon();
}

void AWeapon::EnterInventory()
{
}
void AWeapon::ExitInventory()
{
}

void AWeapon::ServerRPC_Unequip_Implementation()
{
	Unequip();
}
bool AWeapon::ServerRPC_Unequip_Validate()
{
	return true;
}

void AWeapon::Input_PullTrigger()
{
	if (!HasAuthority())
	{
		ServerRPC_PullTrigger();
		return; // TODO Remove return to enable client preditiction (currently broken)
	}
	ReceiverComp->PullTrigger();
}
void AWeapon::ServerRPC_PullTrigger_Implementation()
{
	Input_PullTrigger();
}
bool AWeapon::ServerRPC_PullTrigger_Validate()
{
	return true;
}

void AWeapon::Input_ReleaseTrigger()
{
	if (!HasAuthority())
	{
		ServerRPC_ReleaseTrigger();
		return; // TODO Remove return to enable client preditiction (currently broken)
	}
	ReceiverComp->ReleaseTrigger();
}
void AWeapon::ServerRPC_ReleaseTrigger_Implementation()
{
	Input_ReleaseTrigger();
}
bool AWeapon::ServerRPC_ReleaseTrigger_Validate()
{
	return true;
}

void AWeapon::Input_Reload()
{
	if (!HasAuthority())
	{
		ServerRPC_Reload();
		return; // TODO Remove return to enable client preditiction (currently broken)
	}
	ReceiverComp->Reload();
}
void AWeapon::ServerRPC_Reload_Implementation()
{
	Input_Reload();
}
bool AWeapon::ServerRPC_Reload_Validate()
{
	return true;
}

void AWeapon::Input_AdsPressed()
{
	if (!HasAuthority()) 
	{
		ServerRPC_AdsPressed();
		return; // TODO Remove return to enable client preditiction (currently broken)
	}
	ReceiverComp->AdsPressed();
}
void AWeapon::ServerRPC_AdsPressed_Implementation()
{
	Input_AdsPressed();
}
bool AWeapon::ServerRPC_AdsPressed_Validate()
{
	return true;
}

void AWeapon::Input_AdsReleased()
{
	if (!HasAuthority())
	{
		ServerRPC_AdsReleased();
		return; // TODO Remove return to enable client preditiction (currently broken)
	}
	ReceiverComp->AdsReleased();
}
void AWeapon::ServerRPC_AdsReleased_Implementation()
{
	Input_AdsReleased();
}
bool AWeapon::ServerRPC_AdsReleased_Validate()
{
	return true;
}
bool AWeapon::CanGiveAmmo()
{
	return ReceiverComp->CanGiveAmmo();
}
bool AWeapon::TryGiveAmmo()
{
	return ReceiverComp->TryGiveAmmo();
}





void AWeapon::MultiRPC_NotifyOnShotFired_Implementation()
{
	if (MuzzleLightComp) MuzzleLightComp->SetVisibility(true);

	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, [&]
	{
		if (MuzzleLightComp) MuzzleLightComp->SetVisibility(false);
	}, 0.05, false, -1);

	if (OnShotFired.IsBound()) OnShotFired.Broadcast();
}


void AWeapon::ClientRPC_NotifyOnAmmoWarning_Implementation()
{
	//LogMsgWithRole("AWeapon::ClientRPC_NotifyOnAmmoWarning_Implementation()");
	if (OnAmmoWarning.IsBound()) OnAmmoWarning.Broadcast();
}


/* IReceiverComponentDelegate */
void AWeapon::ShotFired()
{
	MultiRPC_NotifyOnShotFired();
}
void AWeapon::AmmoInClipChanged(int AmmoInClip)
{
	//LogMsgWithRole(FString::Printf(TEXT("AWeapon::AmmoInClipChanged(%d)"), AmmoInClip));

	if (AmmoInClip == 0)
	{
		ClientRPC_NotifyOnAmmoWarning();
	}
}
void AWeapon::AmmoInPoolChanged(int AmmoInPool)
{
	//LogMsgWithRole(FString::Printf(TEXT("AWeapon::AmmoInPoolChanged(%d)"), AmmoInPool));
	if (AmmoInPool == 0)
	{
		ClientRPC_NotifyOnAmmoWarning();
	}
}
void AWeapon::InReloadingChanged(bool IsReloading)
{
	//FString Str{ IsReloading ? "True" : "False" };
	//LogMsgWithRole(FString::Printf(TEXT("AWeapon::InReloadingChanged(%s)"), *FString{ IsReloading ? "True" : "False" }));

	// Use for client side effects only 
	if (HasAuthority()) return;

	if (IsReloading)
	{
		OnReloadStarted.Broadcast();
	}
	else
	{
		OnReloadEnded.Broadcast();
	}
}
void AWeapon::OnReloadProgressChanged(float ReloadProgress)
{
	//LogMsgWithRole(FString::Printf(TEXT("AWeapon::OnReloadProgressChanged(%f)"), ReloadProgress));
}
bool AWeapon::SpawnAProjectile(const FVector& Direction)
{
	check(HasAuthority());
	
	if (ProjectileClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Set a Projectile Class in your Weapon Blueprint to shoot"));
		return false;
	}

	UWorld* World = GetWorld();
	if (World == nullptr) { return false; }



	//const auto ProjectileStartTform = MuzzleLocationComp->GetComponentTransform();

	const auto Hero = Cast<AHeroCharacter>(GetOwningPawn());
	if (!Hero) return false;


	// Offset the aim up or down
	const FRotator DirectionWithPitch{ PitchAimOffset, FMath::RadiansToDegrees(Direction.HeadingAngle()), 0 };
	
	const FTransform SpawnTransform{ DirectionWithPitch, Hero->GetAimTransform().GetLocation() };
	
	// Spawn the projectile at the muzzle.
	auto Projectile = (AProjectile*)UGameplayStatics::BeginDeferredActorSpawnFromClass(this, ProjectileClass, SpawnTransform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	
	if (Projectile == nullptr)
	{
		return false;
	}

	Projectile->SetInstigatingControllerId(HeroControllerId);
	Projectile->Instigator = Instigator;
	Projectile->SetOwner(this);	//Projectile->SetOwner(GetOwner());
	//Projectile->InitVelocity(Direction);

	UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);

	return true;
}
FVector AWeapon::GetBarrelDirection()
{
	FVector AimVec;
	
	const auto Hero = Cast<AHeroCharacter>(GetOwningPawn());
	if (Hero)
	{
		AimVec = Hero->GetAimTransform().GetRotation().Vector();
	}
	else // Plan B
	{
		AimVec = MuzzleLocationComp->GetForwardVector();
	}
	
	return AimVec;
}
FVector AWeapon::GetBarrelLocation()
{
	/*auto Hero = Cast<AHeroCharacter>(GetOwningPawn());
	if (Hero)
		return Hero->GetWeaponAnchor()->GetComponentLocation();*/
	auto Hero = Cast<AHeroCharacter>(GetOwningPawn());
	if (Hero)
		return Hero->GetAimTransform().GetLocation();

	// Plan B
	return MuzzleLocationComp->GetComponentLocation();
}
AActor* AWeapon::GetOwningPawn()
{
	return GetOwner();
}
FString AWeapon::GetWeaponName()
{
	return WeaponName;
}

void AWeapon::CancelAnyReload()
{
	ReceiverComp->CancelAnyReload();
}

float AWeapon::GetDrawDuration()
{
	return GetEquipDuration();
}
/* End IReceiverComponentDelegate */

void AWeapon::LogMsgWithRole(FString message) const
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
FString AWeapon::GetRoleText() const
{
	return GetEnumText(Role) + " " + GetEnumText(GetRemoteRole());
}

