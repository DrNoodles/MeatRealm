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
	void DamageTaken(uint32 InstigatorHeroControllerId, float HealthRemaining, int DamageTaken, bool bHitArmour) const;

	//DECLARE_EVENT_TwoParams(AHeroController, FHealthDepleted, uint32, uint32)
	//FHealthDepleted& OnHealthDepleted() { return HealthDepletedEvent; }

	// receiverId, instigatorId, healthRemaining, damageTaken, isArmour
	DECLARE_MULTICAST_DELEGATE_FiveParams(FTakenDamage, uint32, uint32, int, int, bool)
	FTakenDamage& OnTakenDamage() { return TakenDamageEvent; }


protected:
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual bool InputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad) override;
	virtual bool InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad) override;

private:
	
	FTakenDamage TakenDamageEvent;
	//FHealthDepleted HealthDepletedEvent;

	/// Input
	void Input_MoveUp(float Value);
	void Input_MoveRight(float Value);
	void Input_FaceUp(float Value);
	void Input_FaceRight(float Value);
	void Input_FirePressed();
	void Input_FireReleased();
	void Input_Reload();
	void Input_Interact();
	void SetUseMouseaim(bool bUseMouseAim);
};
