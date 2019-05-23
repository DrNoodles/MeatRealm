// Fill out your copyright notice in the Description page of Project Settings.


#include "HeroController.h"
#include "HeroCharacter.h"

AHeroCharacter* AHeroController::GetHeroCharacter() const
{
	return (AHeroCharacter*)GetCharacter();
}
