// Fill out your copyright notice in the Description page of Project Settings.

#include "Throwable.h"
#include "Components/SkeletalMeshComponent.h"
#include "UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "InventoryComp.h"
#include "HeroCharacter.h"
#include "Projectile.h"


DEFINE_LOG_CATEGORY(LogThrowable);

// Lifecycle //////////////////////////////////////////////////////////////////

AThrowable::AThrowable()
{
	bAlwaysRelevant = true;
	SetReplicates(true);

	PrimaryActorTick.bCanEverTick = true;
	RegisterAllActorTickFunctions(true, false); // necessary for SetActorTickEnabled() to work

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;

	SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMeshComp->SetupAttachment(RootComponent);
	SkeletalMeshComp->SetGenerateOverlapEvents(false);
	SkeletalMeshComp->SetCollisionProfileName(TEXT("NoCollision"));
	SkeletalMeshComp->CanCharacterStepUpOn = ECB_No;
}

void AThrowable::BeginPlay()
{
	LogMsgWithRole("AThrowable::BeginPlay");
	Super::BeginPlay();
	SetActorTickEnabled(false);
}



// Throwing Projectile ///////////////////////////////////////////////////////////

void AThrowable::SpawnProjectile()
{
	LogMsgWithRole("AThrowable::SpawnProjectile()");

	check(HasAuthority());

	if (ProjectileClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Set a Projectile Class in your Throwable Blueprint to spawn it"));
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr) { return; }

	const auto Hero = Cast<AHeroCharacter>(GetOwner());
	if (!Hero) return;


	const auto AimDirection = Hero->GetAimTransform().GetRotation().Vector();
	const auto AimLocation = Hero->GetAimTransform().GetLocation();
	
	// Offset the aim up or down
	const FRotator DirectionWithPitch{ PitchAimOffset, FMath::RadiansToDegrees(AimDirection.HeadingAngle()), 0 };
	
	const FTransform SpawnTransform{ DirectionWithPitch,  AimLocation };

	// Spawn the projectile at the muzzle.
	auto Projectile = (AProjectile*)UGameplayStatics::BeginDeferredActorSpawnFromClass(this, ProjectileClass, SpawnTransform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (Projectile == nullptr)
	{
		return;
	}

	Projectile->SetInstigatingControllerId(InstigatingControllerId);
	Projectile->Instigator = Instigator;
	Projectile->SetOwner(this);

	UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
}

void AThrowable::ProjectileThrown()
{
	LogMsgWithRole("AThrowable::ProjectileThrown()");
}

void AThrowable::ServerRequestThrow_Implementation()
{
	MultiDoThrow();
}
bool AThrowable::ServerRequestThrow_Validate()
{
	return true;
}
void AThrowable::MultiDoThrow_Implementation()
{
	if (HasAuthority())
	{
		// Throw the thing!
		SpawnProjectile();
		Delegate->NotifyItemIsExpended(this);
	}
	else
	{
		// Notify clients of throw for client side effects
		ProjectileThrown();
	}
}
bool AThrowable::MultiDoThrow_Validate()
{
	return true;
}



// Input //////////////////////////////////////////////////////////////////////

void AThrowable::OnPrimaryPressed()
{
	LogMsgWithRole("AThrowable::OnPrimaryPressed()");
	bIsAiming = true;
}
void AThrowable::OnPrimaryReleased()
{
	LogMsgWithRole("AThrowable::OnPrimaryReleased()");

	if (bIsAiming)
	{
		bIsAiming = false;
		ServerRequestThrow();
	}
}

void AThrowable::OnSecondaryPressed()
{
	LogMsgWithRole("AThrowable::OnSecondaryPressed()");
	bIsAiming = false;
}
void AThrowable::OnSecondaryReleased()
{
	LogMsgWithRole("AThrowable::OnSecondaryReleased()");

}



// AEquippableBase ////////////////////////////////////////////////////////////

void AThrowable::EnterInventory()
{
	LogMsgWithRole("AThrowable::EnterInventory");
	check(HasAuthority())
}

void AThrowable::ExitInventory()
{
	LogMsgWithRole("AThrowable::ExitInventory");
	check(HasAuthority())
}

void AThrowable::OnEquipStarted()
{
	// Only run on the authority - TODO Client side prediction, if needed.
	if (!HasAuthority())
	{
		ServerEquipStarted();
		return;
	}

	LogMsgWithRole("AThrowable::OnEquipStarted");

}
void AThrowable::ServerEquipStarted_Implementation()
{
	OnEquipStarted();
}
bool AThrowable::ServerEquipStarted_Validate()
{
	return true;
}

void AThrowable::OnEquipFinished()
{
	// Only run on the authority - TODO Client side prediction, if needed.
	if (!HasAuthority())
	{
		ServerEquipFinished();
		return;
	}

	LogMsgWithRole("AThrowable::OnEquipFinished");
}
void AThrowable::ServerEquipFinished_Implementation()
{
	OnEquipFinished();
}
bool AThrowable::ServerEquipFinished_Validate()
{
	return true;
}

void AThrowable::OnUnEquipStarted()
{
	// Only run on the authority - TODO Client side prediction, if needed.
	if (!HasAuthority())
	{
		ServerUnEquipStarted();
		return;
	}

	LogMsgWithRole("AThrowable::OnUnEquipStarted");
}
void AThrowable::ServerUnEquipStarted_Implementation()
{
	OnUnEquipStarted();
}
bool AThrowable::ServerUnEquipStarted_Validate()
{
	return true;
}

void AThrowable::OnUnEquipFinished()
{
	// Only run on the authority - TODO Client side prediction, if needed.
	if (!HasAuthority())
	{
		ServerUnEquipFinished();
		return;
	}

	LogMsgWithRole("AThrowable::OnUnEquipFinished");
}
void AThrowable::ServerUnEquipFinished_Implementation()
{
	OnUnEquipFinished();
}
bool AThrowable::ServerUnEquipFinished_Validate()
{
	return true;
}



// Helpers ////////////////////////////////////////////////////////////////////

void AThrowable::LogMsgWithRole(FString message) const
{
	FString m = GetRoleText() + ": " + message;
	UE_LOG(LogThrowable, Log, TEXT("%s"), *m);
}
FString AThrowable::GetEnumText(ENetRole role)
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
FString AThrowable::GetRoleText() const
{
	return GetEnumText(Role) + " " + GetEnumText(GetRemoteRole());
}
