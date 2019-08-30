// Fill out your copyright notice in the Description page of Project Settings.

#include "Throwable.h"
#include "Components/SkeletalMeshComponent.h"
#include "UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "InventoryComp.h"
#include "HeroCharacter.h"
#include "Projectile.h"
#include "Components/StaticMeshComponent.h"


DEFINE_LOG_CATEGORY(LogThrowable);

// Lifecycle //////////////////////////////////////////////////////////////////


void AThrowable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Just the owner
	DOREPLIFETIME_CONDITION(AThrowable, bIsAiming, COND_OwnerOnly);
}
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

	HitPreviewMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HitPreviewMesh"));
	HitPreviewMeshComp->SetupAttachment(RootComponent);
	HitPreviewMeshComp->SetGenerateOverlapEvents(false);
	HitPreviewMeshComp->SetCollisionProfileName(TEXT("NoCollision"));
	HitPreviewMeshComp->CanCharacterStepUpOn = ECB_No;
	HitPreviewMeshComp->SetVisibility(false);
}

void AThrowable::BeginPlay()
{
	LogMsgWithRole("AThrowable::BeginPlay");
	Super::BeginPlay();
	SetActorTickEnabled(false);
}
void AThrowable::Tick(float DeltaSeconds)
{
	LogMsgWithRole("AThrowable::Tick");

	if (IsEquipped())
		VisualiseProjectile();
}

FVector AThrowable::GetAimLocation() const
{
	const auto Hero = Cast<AHeroCharacter>(GetOwner());
	if (!Hero)
		return FVector::ZeroVector;

	return Hero->GetAimTransform().GetLocation();
}
FRotator AThrowable::GetAimRotator() const
{
	const auto Hero = Cast<AHeroCharacter>(GetOwner());
	if (!Hero) 
		return FRotator::ZeroRotator;

	const auto AimDirection = Hero->GetAimTransform().GetRotation().Vector();
	const FRotator DirectionWithPitch{ PitchAimOffset, FMath::RadiansToDegrees(AimDirection.HeadingAngle()), 0 };

	return DirectionWithPitch;
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


	const FTransform SpawnTransform{ GetAimRotator(),  GetAimLocation() };

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
	SetAiming(true);
}
void AThrowable::OnPrimaryReleased()
{
	LogMsgWithRole("AThrowable::OnPrimaryReleased()");

	if (bIsAiming)
	{
		SetAiming(false);

		if (IsEquipped())
			ServerRequestThrow();
	}
}

void AThrowable::OnSecondaryPressed()
{
	LogMsgWithRole("AThrowable::OnSecondaryPressed()");
	if (bIsAiming)
	{
		SetAiming(false);
	}
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


// Aiming /////////////////////////////////////////////////////////////////////

void AThrowable::VisualiseProjectile() const
{
	const auto World = GetWorld();
	if (!World)
	{
		return;
	}

	FPredictProjectilePathParams Params;
	FPredictProjectilePathResult OutResult;

	// TODO Optimise! Singleton? Statically cache one of these for the life of the throwable.
	const auto ProjectileInstance = (AProjectile*)UGameplayStatics::BeginDeferredActorSpawnFromClass(World, ProjectileClass, FTransform::Identity);


	// Inputs
	float InitialSpeed = ProjectileInstance->GetInitialSpeed();
	float GravityZ = ProjectileInstance->GetGravityZ();
	float CollisionRadius = ProjectileInstance->GetCollisionRadius();
	Params.StartLocation = GetAimLocation();
	Params.LaunchVelocity = GetAimRotator().Vector() * InitialSpeed;
	Params.OverrideGravityZ = GravityZ;
	Params.ProjectileRadius = CollisionRadius;
	//LogMsgWithRole(FString::Printf(TEXT("InitSpeed %f, Grav %f, Radius: %f"), InitialSpeed, GravityZ, CollisionRadius));

	
	// Config
	Params.bTraceWithChannel = true;
	Params.bTraceWithCollision = true;
	Params.TraceChannel = ECollisionChannel::ECC_Visibility;
	Params.SimFrequency = 30;
	Params.MaxSimTime = 3;

	Params.ActorsToIgnore.Add((AActor*)GetOwner()); // Don't hit big daddy

	Params.DrawDebugType = EDrawDebugTrace::ForOneFrame;

	
	// Trace
	auto DidHit = UGameplayStatics::PredictProjectilePath(World, Params, OUT OutResult);

	
	// Results
	LogMsgWithRole(DidHit ? "Proj Hit" : "Proj Miss");
	LogMsgWithRole(FString::Printf(TEXT("PathData Count %d"), OutResult.PathData.Num()));

	
	// Visualise point of contact
	if (DidHit)
	{
		HitPreviewMeshComp->SetVisibility(true);
		HitPreviewMeshComp->SetWorldLocation(OutResult.HitResult.ImpactPoint);
	}

	
	// Cleanup - TODO Remove need to build one of these each tick. Insanity!
	ProjectileInstance->Destroy();
}

void AThrowable::SetAiming(bool NewAiming)
{
	bIsAiming = NewAiming;
	
	if (!HasAuthority())
	{
		ServerSetAiming(NewAiming);

		// Tick on owning client to enable arc visualisation
		SetActorTickEnabled(NewAiming);

		// Make sure preview mesh is hidden
		if (!NewAiming)
		{
			HitPreviewMeshComp->SetVisibility(false);
		}
	}
}
void AThrowable::ServerSetAiming_Implementation(bool NewAiming)
{
	SetAiming(NewAiming);
}
bool AThrowable::ServerSetAiming_Validate(bool NewAiming)
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
