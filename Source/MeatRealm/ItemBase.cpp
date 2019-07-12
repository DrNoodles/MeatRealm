// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemBase.h"
#include "Engine/World.h"
#include "Interfaces/AffectableInterface.h"
#include "Components/StaticMeshComponent.h"

AItemBase::AItemBase()
{
	bAlwaysRelevant = true;
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;

	SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMeshComp->SetupAttachment(RootComponent);
	SkeletalMeshComp->SetGenerateOverlapEvents(false);
	SkeletalMeshComp->SetCollisionProfileName(TEXT("NoCollision"));
	SkeletalMeshComp->CanCharacterStepUpOn = ECB_No;
}

void AItemBase::EnterInventory()
{
	check(HasAuthority())

}
void AItemBase::ExitInventory()
{
	check(HasAuthority())
	Recipient = nullptr;
}

void AItemBase::UseStart()
{
	if (!HasAuthority())
	{
		ServerUseStart();
	}

	UE_LOG(LogTemp, Warning, TEXT("AItemBase::UseStart"));

	auto UseComplete = [&]
	{
		UE_LOG(LogTemp, Warning, TEXT("AItemBase::UseComplete"));
		ApplyItem(Recipient);
		OnUsageSuccess.Broadcast();
	};

	GetWorldTimerManager().SetTimer(UsageTimerHandle, UseComplete, UsageDuration, false);
}

void AItemBase::UseStop()
{
	if (!HasAuthority())
	{
		ServerUseStop();
	}

	UE_LOG(LogTemp, Warning, TEXT("AItemBase::UseStop"));
	GetWorldTimerManager().ClearTimer(UsageTimerHandle);
}

void AItemBase::SetRecipient(IAffectableInterface* const TheRecipient)
{
	check(HasAuthority())
	Recipient = TheRecipient;
}

void AItemBase::Equip()
{
	if (!HasAuthority())
	{
		ServerEquip();
	}

	UE_LOG(LogTemp, Warning, TEXT("AItemBase::Equip"));
	UseStop();
}

void AItemBase::Unequip()
{
	if (!HasAuthority())
	{
		ServerUnequip();
	}

	UE_LOG(LogTemp, Warning, TEXT("AItemBase::Unequip"));
	UseStop();
}




// Replication 

void AItemBase::ServerUseStart_Implementation()
{
	UseStart();
}
bool AItemBase::ServerUseStart_Validate()
{
	return true;
}
void AItemBase::ServerUseStop_Implementation()
{
	UseStop();
}
bool AItemBase::ServerUseStop_Validate()
{
	return true;
}
void AItemBase::ServerEquip_Implementation()
{
	Equip();
}
bool AItemBase::ServerEquip_Validate()
{
	return true;
}
void AItemBase::ServerUnequip_Implementation()
{
	Unequip();
}
bool AItemBase::ServerUnequip_Validate()
{
	return true;
}