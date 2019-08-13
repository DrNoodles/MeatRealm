// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Structs/DmgHitResult.h" // for DYNAMIC DELEGATES

#include "HeroController.generated.h"

class AHeroCharacter;
class AHeroState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerSpawned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTakenDamage, FMRHitResult, Hit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGivenDamage, FMRHitResult, Hit);

UCLASS()
class MEATREALM_API AHeroController : public APlayerController
{
	GENERATED_BODY()

	// Data

public:
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
		TSubclassOf<class UUserWidget> HudClass;

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
		TSubclassOf<class ADamageNumber> DamageNumberClass;

	UUserWidget* HudInstance;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FPlayerSpawned OnPlayerSpawned;
	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FTakenDamage OnTakenDamage;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FTakenDamage OnGivenDamage;

protected:
private:
	// Used to gate-keep whether player inputs are allowewd! TODO Needs more implementation
	bool bAllowGameActions = true;




	// Methods

public:
	AHeroController();
	void CleanupPlayerState() override;
	bool IsGameInputAllowed() const;

	void OnPossess(APawn* InPawn) override;
	void AcknowledgePossession(APawn* P) override;
	void OnUnPossess() override;
	void InstigatedAnyDamage(float Damage, const UDamageType* DamageType, AActor* DamagedActor, AActor* DamageCauser) override;
	
	AHeroState* GetHeroPlayerState() const;
	AHeroCharacter* GetHeroCharacter() const;
	uint32 GetPlayerId() const;

	void CreateHud();
	void DestroyHud();

	// Debug helpers
	void LogMsgWithRole(FString message);
	FString GetEnumText(ENetRole role);
	FString GetRoleText();
	void TakeDamage2(const FMRHitResult& Hit);
	void SimulateHitGiven(const FMRHitResult& Hit);
	
	UFUNCTION(Client, Reliable)
	void ClientRPC_PlayHit(const FMRHitResult& Hit);

	UFUNCTION(Client, Reliable)
	void ClientRPC_NotifyOnTakenDamage(const FMRHitResult& Hit);
	


	//DECLARE_EVENT_TwoParams(AHeroController, FHealthDepleted, uint32, uint32)
	//FHealthDepleted& OnHealthDepleted() { return HealthDepletedEvent; }

	// receiverId, instigatorId, healthRemaining, damageTaken, isArmour
	//FTakenDamage& OnTakenDamage() { return TakenDamageEvent; }

protected:
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual bool InputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad) override;
	virtual bool InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad) override;

private:
	//FHealthDepleted HealthDepletedEvent;

	/// Input
	void Input_MoveUp(float Value);
	void Input_MoveRight(float Value);
	void Input_FaceUp(float Value);
	void Input_FaceRight(float Value);
	void Input_FirePressed();
	void Input_FireReleased();
	void Input_AdsPressed();
	void Input_AdsReleased();
	void Input_Reload();
	void Input_Interact();
	void Input_PrimaryWeapon();
	void Input_SecondaryWeapon();
	void Input_ToggleWeapon();

	void SetUseMouseaim(bool bUseMouseAim);
};
