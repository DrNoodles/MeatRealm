// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemBase.h"
#include "Engine/World.h"
#include "Interfaces/AffectableInterface.h"

AItemBase::AItemBase()
{
	bAlwaysRelevant = true;
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
}

void AItemBase::BeginDestroy()
{
	Recipient = nullptr;
}


void AItemBase::UseStart(/*IAffectableInterface* Affectable*/)
{
	if (Role < ROLE_Authority)
	{
		ServerUseStart();
	}

	UE_LOG(LogTemp, Warning, TEXT("AItemBase::UseStart"));
	UseStop();

	//auto LocalAffectable = Affectable;
	auto UseComplete = [this]
	{
		UE_LOG(LogTemp, Warning, TEXT("AItemBase::UseComplete"));
		ApplyItem(Recipient);
		OnUsageSuccess.Broadcast();
	};

	GetWorldTimerManager().SetTimer(UsageTimerHandle, UseComplete, UsageDuration, false);
}
void AItemBase::ServerUseStart_Implementation(/*IAffectableInterface* Affectable*/)
{
	UseStart();
}
bool AItemBase::ServerUseStart_Validate(/*IAffectableInterface* Affectable*/)
{
	return true;
}

void AItemBase::UseStop()
{
	UE_LOG(LogTemp, Warning, TEXT("AItemBase::UseStop"));
	GetWorldTimerManager().ClearTimer(UsageTimerHandle);
}

void AItemBase::Equip()
{
	UE_LOG(LogTemp, Warning, TEXT("AItemBase::Equip"));
	UseStop();
}

void AItemBase::Unequip()
{
	UE_LOG(LogTemp, Warning, TEXT("AItemBase::Unequip"));
	UseStop();
}

void AItemBase::SetRecipient(IAffectableInterface* NewAffectable)
{
	check(HasAuthority());

	Recipient = NewAffectable;
}

//
//void AItemBase::BeginPlay()
//{
//	Super::BeginPlay();
//}

//void AItemBase::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//}

