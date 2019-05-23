// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"

#include "HeroController.generated.h"

class AHeroCharacter;

UCLASS()
class MEATREALM_API AHeroController : public APlayerController
{
	GENERATED_BODY()

public:
	AHeroController();


	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
		TSubclassOf<class UUserWidget> wMainMenu;

	// Variable to hold the widget After Creating it.
	UUserWidget* MyMainMenu;

	void BeginPlay() override;
	AHeroCharacter* GetHeroCharacter() const;
	void ShowHud(bool bMakeVisible);
	
};
