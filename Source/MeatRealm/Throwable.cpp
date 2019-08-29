// Fill out your copyright notice in the Description page of Project Settings.


#include "Throwable.h"
#include "UnrealNetwork.h"
#include "Components/SkeletalMeshComponent.h"


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



// Input //////////////////////////////////////////////////////////////////////

void AThrowable::OnPrimaryPressed()
{
}

void AThrowable::OnPrimaryReleased()
{
}

void AThrowable::OnSecondaryPressed()
{
}

void AThrowable::OnSecondaryReleased()
{
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



// Replication ////////////////////////////////////////////////////////////////

void AThrowable::ServerEquipStarted_Implementation()
{
	OnEquipStarted();
}
bool AThrowable::ServerEquipStarted_Validate()
{
	return true;
}
void AThrowable::ServerEquipFinished_Implementation()
{
	OnEquipFinished();
}
bool AThrowable::ServerEquipFinished_Validate()
{
	return true;
}
void AThrowable::ServerUnEquipStarted_Implementation()
{
	OnUnEquipStarted();
}
bool AThrowable::ServerUnEquipStarted_Validate()
{
	return true;
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

