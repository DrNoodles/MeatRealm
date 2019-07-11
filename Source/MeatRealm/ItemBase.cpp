// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemBase.h"
#include "Engine/World.h"

AItemBase::AItemBase()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
}

void AItemBase::UseStart(IAffectableInterface* const Affectable)
{
	UE_LOG(LogTemp, Warning, TEXT("AItemBase::UseStart"));
	UseStop();

	auto UseComplete = [&]
	{
		UE_LOG(LogTemp, Warning, TEXT("AItemBase::UseComplete"));
		ApplyItem(Affectable);
		OnUsageSuccess.Broadcast();
	};

	GetWorldTimerManager().SetTimer(UsageTimerHandle, UseComplete, UsageDuration, false);
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

//
//void AItemBase::BeginPlay()
//{
//	Super::BeginPlay();
//}

//void AItemBase::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//}

