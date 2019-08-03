#include "PickupBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"
#include "UnrealNetwork.h"

void APickupBase::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APickupBase, IsAvailable);
}

APickupBase::APickupBase()
{
	//PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);

	// TODO Introduce USceneComponent so Collision as root can be moved around

	CollisionComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionComp"));
	CollisionComp->InitCapsuleSize(50, 100);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &APickupBase::OnCompBeginOverlap);
	CollisionComp->SetCollisionProfileName(FName("Pickup"));
	RootComponent = CollisionComp;

	SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMeshComp->SetupAttachment(RootComponent);
	SkeletalMeshComp->SetGenerateOverlapEvents(false);
	SkeletalMeshComp->SetCollisionProfileName(TEXT("NoCollision"));
	SkeletalMeshComp->CanCharacterStepUpOn = ECB_No;
}

bool APickupBase::AuthTryInteract(IAffectableInterface* const Affectable)
{
	check(Affectable)
	check(HasAuthority())

	float Delay;
	if (!CanInteract(Affectable, OUT Delay)) return false;
	return TryPickup(Affectable);
}

void APickupBase::OnCompBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//LogMsgWithRole("APickupBase::Overlapping()");
	if (!HasAuthority()) return;

	if (bExplicitInteraction) { return; }

	const auto IsNotWorthChecking = OtherActor == nullptr || OtherActor == this || OtherComp == nullptr;
	if (IsNotWorthChecking) { return; }

	const auto Affectable = Cast<IAffectableInterface>(OtherActor);
	if (Affectable == nullptr) { return; }

	TryPickup(Affectable);
}

void APickupBase::OnRep_IsAvailableChanged()
{
	//LogMsgWithRole("APickupBase::OnRep_IsAvailableChanged()");
	MakePickupAvailable(IsAvailable);
}

bool APickupBase::TryPickup(IAffectableInterface* const Affectable)
{
	//LogMsgWithRole("APickupBase::TryPickup()");
	check(HasAuthority())
	if (!IsAvailable)	return false;
	if (!TryApplyAffect(Affectable)) return false;
	PickupItem();
	return true;
}

void APickupBase::PickupItem()
{
	//LogMsgWithRole("APickupBase::ServerRPC_PickupItem_Implementation()");
	check(HasAuthority())

	MakePickupAvailable(false); // simulate on server
	IsAvailable = false; // replicates to clients


	if (bIsSingleUse)
	{
		// Destroy the pickup after a delay to allow client effects/sounds to finish

		FTimerHandle ThrowAwayHandle;
		const float DestroyDelay = 0.5;

		GetWorld()->GetTimerManager().SetTimer(
			ThrowAwayHandle, [&] { Destroy(); }, DestroyDelay, false, -1);
	}
	else
	{
		// Start respawn timer
		GetWorld()->GetTimerManager().SetTimer(
			RespawnTimerHandle, this, &APickupBase::Respawn, RespawnDelay, false, -1);
	}
}

void APickupBase::Respawn()
{
	//LogMsgWithRole("Respawn()");
	check(HasAuthority())

	// Dispose pickup respawn timer
	if (RespawnTimerHandle.IsValid()) { GetWorld()->GetTimerManager().ClearTimer(RespawnTimerHandle); }

	// Simulate on server
	MakePickupAvailable(true);

	// Notify clients through replicated value
	IsAvailable = true;
}

void APickupBase::MakePickupAvailable(bool bIsAvailable)
{
	//LogMsgWithRole("APickupBase::MakePickupAvailable()");
	if (bIsAvailable)
	{
		// Show visual
		SkeletalMeshComp->SetVisibility(true, true);
	
		// Enable Overlap
		CollisionComp->SetGenerateOverlapEvents(true);

		// Announce availability
		OnSpawn.Broadcast();
	}
	else
	{
		// Disable Overlap
		CollisionComp->SetGenerateOverlapEvents(false);

		// Hide visual
		SkeletalMeshComp->SetVisibility(false, true);

		// Announce taken
		OnTaken.Broadcast();
	}
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
FString APickupBase::GetRoleText()
{
	auto Local = Role;
	auto Remote = GetRemoteRole();
	return GetEnumText(Role) + " " + GetEnumText(GetRemoteRole()) + " Ded:" + (IsRunningDedicatedServer() ? "True" : "False");

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

