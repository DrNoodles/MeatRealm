// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageNumber.h"
#include "WidgetComponent.h"

// Sets default values
ADamageNumber::ADamageNumber()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = 3;

	WidgetComp = CreateDefaultSubobject<UWidgetComponent>("WidgetComp");
	//WidgetComp->SetupAttachment();
	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);

	RootComponent = WidgetComp;
}
