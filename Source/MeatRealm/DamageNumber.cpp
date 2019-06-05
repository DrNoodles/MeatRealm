// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageNumber.h"
#include "WidgetComponent.h"

// Sets default values
ADamageNumber::ADamageNumber()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = 3;

	WidgetComp = CreateDefaultSubobject<UWidgetComponent>("WidgetComp");
	WidgetComp->SetupAttachment(RootComponent);
	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	//WidgetComp->SetRelativeLocation(FVector{ 0,0,200});
	
}
//
//void ADamageNumber::BeginPlay()
//{
//	Super::BeginPlay();
//
//	// TODO Spawn widget on WidgetComponent
////	WidgetComp->SetWidgetClass()
//}
