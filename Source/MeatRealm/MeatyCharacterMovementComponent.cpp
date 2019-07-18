// Fill out your copyright notice in the Description page of Project Settings.


#include "MeatyCharacterMovementComponent.h"
#include "HeroCharacter.h"
#include "Engine/Public/DrawDebugHelpers.h"

float UMeatyCharacterMovementComponent::GetMaxBrakingDeceleration() const
{
	return Super::GetMaxBrakingDeceleration();
}

float UMeatyCharacterMovementComponent::GetMaxSpeed() const
{
	float MaxSpeed = Super::GetMaxSpeed();

	const auto* HeroChar = Cast<AHeroCharacter>(PawnOwner);

	if (HeroChar)
	{
		if (HeroChar->IsTargeting())
		{
			MaxSpeed *= HeroChar->GetTargetingSpeedModifier();
		}

		if (HeroChar->IsUsingItem())
		{
			MaxSpeed = HeroChar->GetHealingMovementSpeed();
		}
		else if (HeroChar->IsRunning())
		{
			if (HeroChar->IsReloading())
			{
				MaxSpeed = HeroChar->GetRunningReloadSpeed();
			}
			else
			{
				MaxSpeed = HeroChar->GetRunningSpeed();
			}
		}
	}

	return MaxSpeed;
}
