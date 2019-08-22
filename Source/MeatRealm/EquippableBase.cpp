// Fill out your copyright notice in the Description page of Project Settings.


#include "EquippableBase.h"


AEquippableBase::AEquippableBase()
{
	PrimaryActorTick.bCanEverTick = true;
}
void AEquippableBase::BeginPlay()
{
	Super::BeginPlay();
}
void AEquippableBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


bool AEquippableBase::Is(EInventoryCategory Category)
{
	return GetInventoryCategory() == Category;
}