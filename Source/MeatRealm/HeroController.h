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
	

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
		TSubclassOf<class UUserWidget> HudClass;

	UUserWidget* HudInstance;
	

	void OnPossess(APawn* InPawn) override;
	void AcknowledgePossession(APawn* P) override;
	void OnUnPossess() override;

	AHeroCharacter* GetHeroCharacter() const;
	void ShowHud(bool bMakeVisible);

	// Debug helpers
	void LogMsgWithRole(FString message);
	FString GetEnumText(ENetRole role);
	FString GetRoleText();
	void HealthDepleted(uint32 InstigatorHeroControllerId) const;

	DECLARE_EVENT_TwoParams(AHeroController, FHealthDepleted, uint32, uint32)
	FHealthDepleted& OnHealthDepleted() { return HealthDepletedEvent; }

protected:
	virtual void SetupInputComponent() override;

private:

	
	FHealthDepleted HealthDepletedEvent;

	/// Input
	void Input_MoveUp(float Value);
	void Input_MoveRight(float Value);
	void Input_FaceUp(float Value);
	void Input_FaceRight(float Value);
	void Input_FirePressed();
	void Input_FireReleased();
	void Input_Reload();
	void Input_Interact();
	void Input_ToggleUseMouse();
	bool bUseMouseAim = true;

};
