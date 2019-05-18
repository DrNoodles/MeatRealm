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

void AWeapon::Shoot()
{
	LogMethodWithRole("Shoot");

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

void AWeapon::PullTrigger()
{
	LogMethodWithRole("PullTrigger");

	//UE_LOG(LogTemp, Warning, TEXT("PullTrigger!"));

	RPC_Fire_OnServer();
	LogMethodWithRole("PulledTrigger");

	if (bRepeats)
	{
		GetWorld()->GetTimerManager().SetTimer(
			CycleTimerHandle, this, &AWeapon::RPC_Fire_OnServer , 1.f / ShotsPerSecond, bRepeats, -1);
	}

}
//
//
//void AWeapon::OnInputFire()
//{
//	LogMethodWithRole("OnInputFire");
//	RPC_Fire_OnServer();
//
//	// TODO Client side prediction of shooting
//	//Shoot();
//}


void AWeapon::ReleaseTrigger()
{
	UE_LOG(LogTemp, Warning, TEXT("ReleaseTrigger!"));

	if (CycleTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(CycleTimerHandle);
	}
}

void AWeapon::RPC_Fire_OnServer_Implementation()
{
	LogMethodWithRole("RPC_Fire_OnServer_Impl");
	RPC_Fire_RepToClients();
}

bool AWeapon::RPC_Fire_OnServer_Validate()
{
	return true;
}

void AWeapon::RPC_Fire_RepToClients_Implementation()
{
	LogMethodWithRole("RPC_Fire_RepToClients_Impl");
	Shoot();
}




void AWeapon::LogMethodWithRole(FString message)
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

