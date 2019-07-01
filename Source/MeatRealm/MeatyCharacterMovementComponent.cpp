// Fill out your copyright notice in the Description page of Project Settings.


#include "MeatyCharacterMovementComponent.h"
#include "HeroCharacter.h"


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

		if (HeroChar->IsRunning())
		{
			MaxSpeed *= HeroChar->GetRunningSpeedModifier();
		}
	}

	return MaxSpeed;
}
