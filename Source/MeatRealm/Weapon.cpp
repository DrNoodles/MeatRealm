// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "Engine/World.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"


// Sets default values
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
/*
	ShotSpawnLocation = CreateDefaultSubobject<UArrowComponent>(TEXT("ShotSpawnLocation"));
	ShotSpawnLocation->SetupAttachment(RootComponent);*/

	MuzzleLocationComp = CreateDefaultSubobject<UArrowComponent>(TEXT("MuzzleLocationComp"));
	MuzzleLocationComp->SetupAttachment(RootComponent);

	// TODO Show a billboard if by default on the placeholder
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::PullTrigger()
{
	//UE_LOG(LogTemp, Warning, TEXT("PullTrigger!"));

	OnFire();

	if (bRepeats)
	{
		GetWorld()->GetTimerManager().SetTimer(
			CycleTimerHandle, this, &AWeapon::OnFire, 1.f / ShotsPerSecond, bRepeats, -1);
	}

}

void AWeapon::ReleaseTrigger()
{
	//UE_LOG(LogTemp, Warning, TEXT("ReleaseTrigger!"));

	if (CycleTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CycleTimerHandle);
	}
}

void AWeapon::OnFire()
{
	
	if (ProjectileClass == nullptr) return;
	// TODO Set an error message in log suggesting the designer set a projectile class

	UWorld* World = GetWorld();
	if (World == nullptr) return;

	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Instigator;

	// Spawn the projectile at the muzzle.
	AProjectile* Projectile = GetWorld()->SpawnActorAbsolute<AProjectile>(
		ProjectileClass,
		MuzzleLocationComp->GetComponentTransform(), SpawnParams);

	// Set the projectile velocity
	if (Projectile == nullptr) return;

	const auto AdditionalVelocity = GetOwner()->GetVelocity(); // inherits player's velocity
	const auto ShootDirection = MuzzleLocationComp->GetForwardVector();

	Projectile->FireInDirection(ShootDirection, FVector::ZeroVector);
	//UE_LOG(LogTemp, Warning, TEXT("Fired!"));
}

