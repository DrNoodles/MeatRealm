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
	
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);

	ShotSpawnLocation = CreateDefaultSubobject<UArrowComponent>(TEXT("ShotSpawnLocation"));
	ShotSpawnLocation->SetupAttachment(RootComponent);

	ShotSpawnLocation2 = CreateDefaultSubobject<UArrowComponent>(TEXT("ShotSpawnLocation2"));
	ShotSpawnLocation2->SetupAttachment(RootComponent);

}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AWeapon::Tick(float DeltaTime)
{
	//Super::Tick(DeltaTime);
}

void AWeapon::PullTrigger()
{
	UE_LOG(LogTemp, Warning, TEXT("PullTrigger!"));

	OnFire();

	if (bRepeats)
	{
		GetWorld()->GetTimerManager().SetTimer(
			CycleTimerHandle, this, &AWeapon::OnFire, ShotsPerSecond, bRepeats, -1);
	}

}

void AWeapon::ReleaseTrigger()
{
	UE_LOG(LogTemp, Warning, TEXT("ReleaseTrigger!"));

	if (CycleTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CycleTimerHandle);
	}
}

void AWeapon::OnFire()
{
	UE_LOG(LogTemp, Warning, TEXT("Fire!"));
	
	if (ProjectileClass == nullptr) return;

	UWorld* World = GetWorld();
	if (World == nullptr) return;

	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = Instigator;

	// Spawn the projectile at the muzzle.
	AProjectile* Projectile = GetWorld()->SpawnActorAbsolute<AProjectile>(
		ProjectileClass,
		ShotSpawnLocation2->GetComponentTransform());


	//AProjectile* Projectile = nullptr;// = GetWorld()->SpawnActorAbsolute<AProjectile>(
		//ProjectileClass,
		//ShotSpawnLocation->GetComponentTransform());

	/*AProjectile* Projectile = World->SpawnActor<AProjectile>(
		ProjectileClass, ShotSpawnLocation->GetComponentTransform(), SpawnParams);*/

	/*AProjectile* Projectile = World->SpawnActor<AProjectile>(
		ProjectileClass, MuzzleLocation, MuzzleRotation, SpawnParams);*/

	if (Projectile == nullptr) return;

	Projectile->FireInDirection(ShotSpawnLocation2->GetForwardVector());
}

