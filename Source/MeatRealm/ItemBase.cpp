// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemBase.h"
#include "Engine/World.h"
#include "Interfaces/AffectableInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "HeroCharacter.h"
#include "UnrealNetwork.h"
#include "InventoryComp.h"

AItemBase::AItemBase()
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

void AItemBase::EnterInventory()
{
	check(HasAuthority())
}
void AItemBase::ExitInventory()
{
	check(HasAuthority())
	Recipient = nullptr;
	Delegate = nullptr;
}

void AItemBase::BeginPlay()
{
	UE_LOG(LogTemp, Warning, TEXT("AItemBase::BeginPlay"));
	Super::BeginPlay();
	//SetActorTickInterval(.1); // tick every 10th of a sec
	SetActorTickEnabled(false);
}

float AItemBase::GetUsageTimeRemaining() const
{
	return (1 - UsageProgress) * UsageDuration;
}

void AItemBase::Tick(float DT)
{
	if (HasAuthority()) return;

	// Do client side progress update
	if (bIsInUse)
	{
		const auto ElapsedReloadTime = (FDateTime::Now() - UsageStartTime).GetTotalSeconds();
		UsageProgress = ElapsedReloadTime / UsageDuration;
		//UE_LOG(LogTemp, Warning, TEXT("AItemBase::Tick - Progress:%f"), UsageProgress);
	}
}

void AItemBase::UsePressed()
{
	if (!HasAuthority())
	{
		ServerUsePressed();
	}

	UE_LOG(LogTemp, Warning, TEXT("AItemBase::UsePressed"));


	if (bIsInUse) return;
	

	if (!CanApplyItem(Recipient))
	{
		OnUsageDenied.Broadcast();
		return;
	}

	// Start the usage!
	SetActorTickEnabled(true);
	bIsInUse = true;
	UsageStartTime = FDateTime::Now();
	UsageProgress = 0;
	OnUsageStarted.Broadcast();

	GetWorldTimerManager().SetTimer(UsageTimerHandle, this, &AItemBase::UseComplete, UsageDuration, false);
}

void AItemBase::UseComplete()
{
	UE_LOG(LogTemp, Warning, TEXT("AItemBase::UseComplete"));

	bIsInUse = false;
	UsageProgress = 100;
	SetActorTickEnabled(false);

	if (OnUsageSuccess.IsBound()) OnUsageSuccess.Broadcast(); // There was a crash here... Not sure the issue

	if (HasAuthority())
	{
		ApplyItem(Recipient);

		UE_LOG(LogTemp, Warning, TEXT("AItemBase::UseComplete - NotifyEquippableIsExpended"));
		ensure(Delegate);
		Delegate->NotifyItemIsExpended(this);
	}
}

void AItemBase::UseReleased()
{
	if (!bIsHoldToUse) return;

	if (!HasAuthority())
	{
		ServerUseReleased();
	}

	UE_LOG(LogTemp, Warning, TEXT("AItemBase::UseReleased"));
	StopAnyUsage();

}

void AItemBase::Cancel()
{
	if (!HasAuthority())
	{
		ServerCancel();
	}

	UE_LOG(LogTemp, Warning, TEXT("AItemBase::Cancel"));
	StopAnyUsage();
}

void AItemBase::StopAnyUsage()
{
	if (!bIsInUse) return;

	GetWorldTimerManager().ClearTimer(UsageTimerHandle);
	SetActorTickEnabled(false);
	bIsInUse = false;
	UsageProgress = 0;

	OnUsageCancelled.Broadcast();
}

void AItemBase::SetRecipient(IAffectableInterface* const TheRecipient)
{
	check(HasAuthority())
	Recipient = TheRecipient;
	ClientSetRecipient(Cast<UObject>(TheRecipient));
}

void AItemBase::OnEquipStarted()
{
	
}

void AItemBase::OnEquipFinished()
{
	if (!HasAuthority())
	{
		ServerEquip();
	}

	UE_LOG(LogTemp, Warning, TEXT("AItemBase::Equip"));
	StopAnyUsage();

	// Immedaitely start using it (HACKY) TODO Integrate this properly if the test works
	if (!bIsHoldToUse && bIsAutoUseOnEquip)
	{
		UsePressed();
	}
}

void AItemBase::OnUnEquipStarted()
{
	if (!HasAuthority())
	{
		ServerUnequip();
	}

	UE_LOG(LogTemp, Warning, TEXT("AItemBase::Unequip"));
	StopAnyUsage();
}

void AItemBase::OnUnEquipFinished()
{
}



// Replication 
void AItemBase::ServerUsePressed_Implementation()
{
	UsePressed();
}
bool AItemBase::ServerUsePressed_Validate()
{
	return true;
}
void AItemBase::ServerUseReleased_Implementation()
{
	UseReleased();
}
bool AItemBase::ServerUseReleased_Validate()
{
	return true;
}
void AItemBase::ServerCancel_Implementation()
{
	Cancel();
}
bool AItemBase::ServerCancel_Validate()
{
	return true;
}
void AItemBase::ServerEquip_Implementation()
{
	OnEquipFinished();
}
bool AItemBase::ServerEquip_Validate()
{
	return true;
}
void AItemBase::ServerUnequip_Implementation()
{
	OnUnEquipStarted();
}
bool AItemBase::ServerUnequip_Validate()
{
	return true;
}

void AItemBase::ClientSetRecipient_Implementation(UObject* Affectable)
{
	const auto AsIFace = Cast<IAffectableInterface>(Affectable);
	if (AsIFace)
	{
		Recipient = AsIFace;
	}
}
