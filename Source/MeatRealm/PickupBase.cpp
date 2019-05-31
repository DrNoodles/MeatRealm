// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Public/TimerManager.h"
#include "UnrealNetwork.h"

// Sets default values
APickupBase::APickupBase()
{
	//PrimaryActorTick.bCanEverTick = true;

	SetReplicates(true);

	// TODO Introduce USceneComponent so Collision as root can be moved around

	CollisionComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionComp"));
	CollisionComp->InitCapsuleSize(50, 100);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &APickupBase::OnCompBeginOverlap);
	RootComponent = CollisionComp;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetGenerateOverlapEvents(false);
	MeshComp->SetCollisionProfileName(TEXT("NoCollision"));
	MeshComp->CanCharacterStepUpOn = ECB_No;

}

bool APickupBase::TryInteract(IAffectableInterface* const Affectable)
{
	if (!bExplicitInteraction || Affectable == nullptr) return false;
	return TryPickup(Affectable);
}


void APickupBase::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APickupBase, IsAvailable);
}


bool APickupBase::TryPickup(IAffectableInterface* const Affectable)
{
	LogMsgWithRole("APickupBase::TryPickup()");

	if (!IsAvailable || !HasAuthority())
	{
		return false;
	}

	if (!TryApplyAffect(Affectable))
	{
		LogMsgWithRole("APickupBase::TryPickup() : Cannot TryApplyAffect()");
		return false;
	}

	ServerRPC_PickupItem();
	LogMsgWithRole("APickupBase::TryPickup() : Success!");

	return true;
}

void APickupBase::OnCompBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	LogMsgWithRole("APickupBase::Overlapping()");

	if (bExplicitInteraction) { return; }

	const auto IsNotWorthChecking = OtherActor == nullptr || OtherActor == this || OtherComp == nullptr;
	if (IsNotWorthChecking) { return; }

	const auto Affectable = Cast<IAffectableInterface>(OtherActor);
	if (Affectable == nullptr) { return; }
	
	TryPickup(Affectable);
}


void APickupBase::ServerRPC_PickupItem_Implementation()
{
	LogMsgWithRole("APickupBase::ServerRPC_PickupItem_Implementation()");

	MakePickupAvailable(false); // simulate on server
	IsAvailable = false; // replicates to clients


	// Start respawn timer
	GetWorld()->GetTimerManager().SetTimer(
		RespawnTimerHandle, this, &APickupBase::Respawn, RespawnDelay, false, -1);
}

bool APickupBase::ServerRPC_PickupItem_Validate()
{
	return true;
}

void APickupBase::OnRep_IsAvailableChanged()
{
	LogMsgWithRole("APickupBase::OnRep_IsAvailableChanged()");
	MakePickupAvailable(IsAvailable);
}

void APickupBase::MakePickupAvailable(bool bIsAvailable)
{
	LogMsgWithRole("APickupBase::MakePickupAvailable()");
	if (bIsAvailable)
	{
		// Show visual
		MeshComp->SetVisibility(true, true);
	
		// Enable Overlap
		CollisionComp->SetGenerateOverlapEvents(true);
	}
	else
	{
		// Disable Overlap
		CollisionComp->SetGenerateOverlapEvents(false);

		// Hide visual
		MeshComp->SetVisibility(false, true);
	}
}

void APickupBase::Respawn()
{
	LogMsgWithRole("Respawn()");

	// Dispose pickup respawn timer
	if (RespawnTimerHandle.IsValid()) { GetWorld()->GetTimerManager().ClearTimer(RespawnTimerHandle); }

	// Simulate on server
	MakePickupAvailable(true);

	// Notify clients through replicated value
	IsAvailable = true;
}









void APickupBase::LogMsgWithRole(FString message)
{
	FString m = GetRoleText() + ": " + message;
	UE_LOG(LogTemp, Warning, TEXT("%s"), *m);
}
FString APickupBase::GetEnumText(ENetRole role)
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
FString APickupBase::GetRoleText()
{
	auto Local = Role;
	auto Remote = GetRemoteRole();


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

