// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageNumber.h"
#include "WidgetComponent.h"
#include "Components/SceneComponent.h"

ADamageNumber::ADamageNumber()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = 3;

	// Create a follow camera offset node
	OffsetComp = CreateDefaultSubobject<USceneComponent>(TEXT("OffsetComp"));
	RootComponent = OffsetComp;


	WidgetComp = CreateDefaultSubobject<UWidgetComponent>("WidgetComp");
	WidgetComp->SetupAttachment(OffsetComp);
	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);

}
