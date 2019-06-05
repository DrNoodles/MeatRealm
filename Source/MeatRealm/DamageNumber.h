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
	void SetDamage(int DamageIn) { Damage = DamageIn; }
	void SetHitArmour(bool HitArmourIn) { bHitArmour = HitArmourIn; }


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		int Damage = -123;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		bool bHitArmour = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UWidgetComponent* WidgetComp = nullptr;
private:


};
