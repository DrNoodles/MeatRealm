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


void AWeapon::BeginPlay()
{
	Super::BeginPlay();
}

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

void AWeapon::Draw()
{
	ReceiverComp->RequestResume();
}

void AWeapon::QueueHolster()
{
	ReceiverComp->RequestPause();
}

//
//void AWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
//{
//	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//}

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



/// INPUT

void AWeapon::Input_PullTrigger()
{
	ReceiverComp->Input_PullTrigger();
}

void AWeapon::Input_ReleaseTrigger()
{
	ReceiverComp->Input_ReleaseTrigger();
}

void AWeapon::Input_Reload()
{
	ReceiverComp->Input_Reload();
}

void AWeapon::Input_AdsPressed()
{
	ReceiverComp->Input_AdsPressed();
}

void AWeapon::Input_AdsReleased()
{
	ReceiverComp->Input_AdsReleased();
}

bool AWeapon::TryGiveAmmo()
{
	return ReceiverComp->TryGiveAmmo();
}


/* IReceiverComponentDelegate */
void AWeapon::ShotFired()
{
	MultiRPC_NotifyOnShotFired();
}
void AWeapon::AmmoInClipChanged(int AmmoInClip)
{
	LogMsgWithRole(FString::Printf(TEXT("AWeapon::AmmoInClipChanged(%d)"), AmmoInClip));

	if (AmmoInClip == 0)
	{
		ClientRPC_NotifyOnAmmoWarning();
	}
}
void AWeapon::AmmoInPoolChanged(int AmmoInPool)
{
	LogMsgWithRole(FString::Printf(TEXT("AWeapon::AmmoInPoolChanged(%d)"), AmmoInPool));
	if (AmmoInPool == 0)
	{
		ClientRPC_NotifyOnAmmoWarning();
	}
}
void AWeapon::InReloadingChanged(bool IsReloading)
{
	//FString Str{ IsReloading ? "True" : "False" };
	LogMsgWithRole(FString::Printf(TEXT("AWeapon::InReloadingChanged(%s)"), *FString{ IsReloading ? "True" : "False" }));

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
	LogMsgWithRole(FString::Printf(TEXT("AWeapon::OnReloadProgressChanged(%f)"), ReloadProgress));
}
bool AWeapon::SpawnAProjectile(const FVector& Direction)
{
	if (ProjectileClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Set a Projectile Class in your Weapon Blueprint to shoot"));
		return false;
	}
	
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
FVector AWeapon::GetBarrelDirection()
{
	return MuzzleLocationComp->GetForwardVector();
}
FVector AWeapon::GetBarrelLocation()
{
	return MuzzleLocationComp->GetComponentLocation();
}
/* End IReceiverComponentDelegate */

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

