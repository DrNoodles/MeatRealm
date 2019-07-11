// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemBase.h"
#include "Engine/World.h"
#include "TimerManager.h"

AItemBase::AItemBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AItemBase::StartUse(IAffectableInterface* const Affectable)
{
	StopUse();

	//auto TM = GetWorld()->GetTimerManager();
	//TM.SetTimer()
}

void AItemBase::StopUse()
{
}

void AItemBase::Equip()
{
	StopUse();
}

void AItemBase::Unequip()
{
	StopUse();
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

