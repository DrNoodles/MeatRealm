// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "DeathmatchGameMode.h"
#include "HeroCharacter.h" // TODO Make this a forward decl - Need to pull FMRHitResult out of the file
#include "DamageNumber.h"

#include "HeroController.generated.h"

class AHeroCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerSpawned);

UCLASS()
class MEATREALM_API AHeroController : public APlayerController
{
	GENERATED_BODY()

public:

	AHeroController();
	void CleanupPlayerState() override;

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
		TSubclassOf<class UUserWidget> HudClass;

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
		TSubclassOf<class ADamageNumber> DamageNumberClass;

	UUserWidget* HudInstance;
	

	void OnPossess(APawn* InPawn) override;
	void AcknowledgePossession(APawn* P) override;
	void OnUnPossess() override;

	AHeroCharacter* GetHeroCharacter() const;
	void CreateHud();
	void DestroyHud();

	// Debug helpers
	void LogMsgWithRole(FString message);
	FString GetEnumText(ENetRole role);
	FString GetRoleText();
	void DamageTaken(const FMRHitResult& Hit) const;
	void SimulateHitGiven(const FMRHitResult& Hit);
	
	UFUNCTION(Client, Reliable)
	void ClientRPC_PlayHit(const FMRHitResult& Hit);

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FPlayerSpawned OnPlayerSpawned;

	//DECLARE_EVENT_TwoParams(AHeroController, FHealthDepleted, uint32, uint32)
	//FHealthDepleted& OnHealthDepleted() { return HealthDepletedEvent; }

	// receiverId, instigatorId, healthRemaining, damageTaken, isArmour
	DECLARE_MULTICAST_DELEGATE_OneParam(FTakenDamage, FMRHitResult)
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
