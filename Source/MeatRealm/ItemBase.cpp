// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemBase.h"
#include "Engine/World.h"
#include "Interfaces/AffectableInterface.h"
#include "Components/StaticMeshComponent.h"
#include "UnrealNetwork.h"
#include "HeroCharacter.h"
#include "Interfaces/EquippableDelegate.h"

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
}

void AItemBase::SetDelegate(AHeroCharacter* NewDelegate)
{
	UE_LOG(LogTemp, Warning, TEXT("AItemBase::SetDelegate  Delegate set"));
	check (HasAuthority())

	Delegate = NewDelegate;
	
	//IEquippableDelegate* I = (IEquippableDelegate*)NewDelegate;
	//if (I)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("AItemBase::SetDelegate  Delegate set"));
	//	Delegate = I;

	//}
	//else
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("AItemBase::SetDelegate - fail"));

	//}


}

void AItemBase::BeginPlay()
{
	//SetActorTickInterval(.1); // tick every 10th of a sec
	SetActorTickEnabled(false);
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

void AItemBase::UseStart()
{

	if (!HasAuthority())
	{
		ServerUseStart();
	}

	UE_LOG(LogTemp, Warning, TEXT("AItemBase::UseStart"));


	
	if (!CanApplyItem(Recipient)) return;

	// Start the usage!
	SetActorTickEnabled(true);
	bIsInUse = true;
	UsageStartTime = FDateTime::Now();
	UsageProgress = 0;


	// Use complete function - TODO Break this out into its own private method
	auto UseComplete = [&]
	{
		UE_LOG(LogTemp, Warning, TEXT("AItemBase::UseComplete"));
		
		

	//	OnUsageSuccess.Broadcast();

		bIsInUse = false;
		UsageProgress = 100;
		SetActorTickEnabled(false);

		if (HasAuthority())
		{
			ApplyItem(Recipient);
			if (Delegate)
			{
				UE_LOG(LogTemp, Warning, TEXT("AItemBase::UseComplete - NotifyEquippableIsExpended"));
				Delegate->NotifyItemIsExpended(this);
			}
		}
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
	SetActorTickEnabled(false);
	bIsInUse = false;
	UsageProgress = 0;
}

void AItemBase::SetRecipient(IAffectableInterface* const TheRecipient)
{
	check(HasAuthority())
	Recipient = TheRecipient;
	ClientSetRecipient(Cast<UObject>(TheRecipient));
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
//void AItemBase::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
//{
//	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//	DOREPLIFETIME_CONDITION(AItemBase, Recipient, COND_OwnerOnly);
//}
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

void AItemBase::ClientSetRecipient_Implementation(UObject* Affectable)
{
	const auto AsIFace = Cast<IAffectableInterface>(Affectable);
	if (AsIFace)
	{
		Recipient = AsIFace;
	}
}
