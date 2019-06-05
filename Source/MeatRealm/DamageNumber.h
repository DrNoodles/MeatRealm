// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "DamageNumber.generated.h"

class UWidgetComponent;

UCLASS()
class MEATREALM_API ADamageNumber : public AActor
{
	GENERATED_BODY()
	
public:	
	ADamageNumber();

protected:
	//virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	UWidgetComponent* WidgetComp = nullptr;
};
