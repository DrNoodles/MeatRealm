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
	void SetDamage(int Damage) { this->Damage = Damage; }
	void SetHitArmour(bool bHitArmour) { this->bHitArmour; }

private:
	UPROPERTY(VisibleAnywhere)
	UWidgetComponent* WidgetComp = nullptr;

	int Damage = 0;
	bool bHitArmour = false;
};
