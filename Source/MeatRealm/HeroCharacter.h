// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/AffectableInterface.h"
#include "InventoryComp.h" // For IInventoryCompDelegate - TODO Split that interface out of InventoryComp for leaner compiling
#include "Structs/AimInfo.h"

#include "HeroCharacter.generated.h"

class AHeroState;
class AHeroController;
class AWeapon;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerTintChanged);

UCLASS()
class MEATREALM_API AHeroCharacter : public ACharacter, public IAffectableInterface, public IInventoryCompDelegate
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Camera)
		bool bLeanCameraWithAim = true;

	UPROPERTY(EditAnywhere, Category = Camera, meta = (EditCondition = "bLeanCameraWithAim"))
		float LeanDistance = 300;

	UPROPERTY(EditAnywhere, Category = Camera, meta = (EditCondition = "bLeanCameraWithAim"))
		float LeanCushionRateGamepad = 2.5;

	UPROPERTY(EditAnywhere, Category = Camera, meta = (EditCondition = "bLeanCameraWithAim"))
		float LeanCushionRateMouse = 5;

	UPROPERTY(EditAnywhere, Category = Camera, meta = (EditCondition = "bLeanCameraWithAim"))
		int ClippingModeMouse = 1;

	UPROPERTY(EditAnywhere, Category = Camera, meta = (EditCondition = "bLeanCameraWithAim"))
		bool bUseExperimentalMouseTracking = false;

	UPROPERTY(EditAnywhere)
	float InteractableSearchDistance = 150.f; //cm

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float MaxHealth = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float MaxArmour = 100.f;

	UPROPERTY(BlueprintReadOnly, Replicated)
		float Health = 100.f;

	UPROPERTY(BlueprintReadOnly, Replicated)
		float Armour = 0.f;

	UPROPERTY(EditAnywhere)
		float RunningSpeed = 525;

	UPROPERTY(EditAnywhere)
		float RunningReloadSpeed = 425;

	UPROPERTY(EditAnywhere)
		float HealingMovementSpeed = 250;

	UPROPERTY(EditAnywhere)
		float WalkSpeed = 375;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FPlayerTintChanged OnPlayerTintChanged;

	// 
	UPROPERTY(EditAnywhere)
		int SprintMaxAngle = 70;

	// An angle between 0 and 180 degrees. 
	UPROPERTY(EditAnywhere)
	int BackpedalThresholdAngle = 135;

	UPROPERTY(EditAnywhere)
	float BackpedalSpeedMultiplier = 0.6;

	// Degrees per second
	UPROPERTY(EditAnywhere)
		float RunTurnRateBase = 90;

	UPROPERTY(EditAnywhere)
		float RunTurnRateMax = 360;

	// Seconds until an action works after running 
	UPROPERTY(EditAnywhere)
		float RunCooldown = 0.4;

	UPROPERTY(EditAnywhere)
	bool bCancelReloadOnRun = false;

	// Not replicated cuz diff local vs server time;
	FDateTime LastRunEnded;

	FTimerHandle QueuePrimaryAfterRunTimerHandle;
	FTimerHandle QueueSecondaryAfterRunTimerHandle;

	UPROPERTY(EditAnywhere)
	float Deadzone = 0.3;


	UPROPERTY(EditAnywhere, Category = Debug)
		bool bDrawMovementInput;

	UPROPERTY(EditAnywhere, Category = Debug)
		bool bDrawMovementVector;

	UPROPERTY(EditAnywhere, Category = Debug)
		bool bDrawMovementSpeed;
	
	FAimInfo LastAimInfo;


protected:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TintChanged)
		FColor TeamTint = FColor::Black;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UStaticMeshComponent* AimPosComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UInventoryComp* InventoryComp = nullptr;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UArrowComponent* WeaponAnchor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UArrowComponent* HolsteredweaponAnchor = nullptr;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class USceneComponent* FollowCameraOffsetComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* FollowCamera;

	bool bUseMouseAim = true;
	float AxisMoveUp;
	float AxisMoveRight;
	float AxisFaceUp;
	float AxisFaceRight;

	FVector2D AimPos_ScreenSpace = FVector2D::ZeroVector;
	FVector AimPos_WorldSpace = FVector::ZeroVector;

	bool bWantsToUsePrimary;
	bool bWantsToUseSecondary;


	UPROPERTY(Transient, Replicated)
	bool bIsRunning = false;

	const char* HandSocketName = "HandSocket";
	const char* Holster1SocketName = "Holster1Socket";
	const char* Holster2SocketName = "Holster2Socket";


public:
	AHeroCharacter(const FObjectInitializer& ObjectInitializer);
	void Restart() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	void SetTint(FColor bCond)
	{
		TeamTint = bCond;

		// Listen server
		if (HasAuthority() && GetNetMode() != NM_DedicatedServer)
		{
			OnRep_TintChanged();
		}
	}
	FColor GetTint() const { return TeamTint; }

	float GetHealingMovementSpeed() const { return HealingMovementSpeed; }

	virtual void PostInitializeComponents() override;
	
	virtual bool ShouldTakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/* IInventoryCompDelegate */
	uint32 GetControllerId() override;
	FTransform GetHandSocketTransform() override;
	void RefreshWeaponAttachments() override;
	/* End IInventoryCompDelegate */
	

	/* IAffectableInterface */
	UFUNCTION()
	bool CanGiveHealth() override;
	UFUNCTION()
	bool AuthTryGiveHealth(float Hp) override;
	UFUNCTION()
	bool CanGiveArmour() override;
	UFUNCTION()
	bool AuthTryGiveArmour(float Delta) override;
	UFUNCTION()
	bool CanGiveAmmo() override;
	UFUNCTION()
	bool AuthTryGiveAmmo() override;
	UFUNCTION()
	bool AuthTryGiveWeapon(const TSubclassOf<AWeapon>& Class, FWeaponConfig& Config) override;
	UFUNCTION()
	bool CanGiveWeapon(const TSubclassOf<AWeapon>& Class, float& OutDelay) override;
	
	// TODO Add EInventoryCategory as param to optimise checks. 
	UFUNCTION()
	bool CanGiveItem(const TSubclassOf<AItemBase>& Class, float& OutDelay) override;
	UFUNCTION()
	bool TryGiveItem(const TSubclassOf<AItemBase>& Class) override;

	UFUNCTION()
		bool CanGiveThrowable(const TSubclassOf<AThrowable>& Class, float& OutDelay) override;
	UFUNCTION()
		bool TryGiveThrowable(const TSubclassOf<AThrowable>& Class) override;
	/* End IAffectableInterface */


	FTransform GetAimTransform() const;

	AHeroState* GetHeroState() const;
	AHeroController* GetHeroController() const;
	float GetTargetingSpeedModifier() const;
	bool IsReloading() const;
	bool IsTargeting() const;
	bool IsUsingItem() const;


	static bool IsBackpedaling(const FVector& MoveDir, const FVector& AimDir, int BackpedalAngle);


	/// Input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	void StartPrimaryAction();
	void StopPrimaryAction();
	void StartSecondaryAction();
	void StopSecondaryAction();

	void Input_PrimaryPressed();
	void Input_PrimaryReleased();
	void Input_SecondaryPressed();
	void Input_SecondaryReleased();
	void Input_Reload() const;

	void Input_MoveUp(float Value) {	AxisMoveUp = Value; }
	void Input_MoveRight(float Value) { AxisMoveRight = Value; }
	void Input_FaceUp(float Value) { AxisFaceUp = Value; }
	void Input_FaceRight(float Value) { AxisFaceRight = Value; }
	void Input_Interact();
	void OnEquipPrimaryWeapon();
	void OnEquipSecondaryWeapon();

	void OnToggleWeapon();


	void SetUseMouseAim(bool bUseMouseAimIn) { bUseMouseAim = bUseMouseAimIn; }



	bool IsRunning() const { return bIsRunning; }
	//bool IsTargeting() const;
	float GetRunningSpeed() const { return RunningSpeed; }
	
	float GetRunningReloadSpeed() const { return RunningReloadSpeed; }

	void AuthOnDeath();

private:

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void MultiOnDeath();
	
	void OnDeathImpl();
	
	void SetRagdollPhysics();

	void ScanForWeaponPickups(float DeltaSeconds);
	virtual void Tick(float DeltaSeconds) override;
	void TickWalking(float DT);
	void TickRunning(float DT);

	void OnEquipSmartHeal();
	void EquipSmartHeal();
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerEquipSmartHeal();

	void OnEquipHealth();
	void EquipHealth();
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEquipHealth();

	void OnEquipArmour();
	void EquipArmour();
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerEquipArmour();

	void OnEquipThrowable();
	void EquipThrowable();
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerEquipThrowable();

	
	void OnRunToggle();
	void OnStartRunning();
	void OnStopRunning();
	void SetRunning(bool bNewIsRunning);

	
	static FVector2D GetGameViewportSize();
	static FVector2D CalcLinearLeanVectorUnclipped(const FVector2D& CursorLoc, const FVector2D& ViewportSize);
	void MoveCameraByOffsetVector(const FVector2D& Vector2D, float DeltaSeconds) const;
	FVector2D TrackCameraWithAimMouse() const;
	FVector2D TrackCameraWithAimGamepad() const;
	void ExperimentalMouseAimTracking(float DT);

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerSetRunning(bool bNewWantsToRun);

	UFUNCTION()
		void OnRep_TintChanged() const;

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_TryInteract();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerEquipPrimaryWeapon();
	
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerEquipSecondaryWeapon();


	void ToggleWeapon();
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerToggleWeapon();


	template<class T>
	T* ScanForInteractable();

	FHitResult GetFirstPhysicsBodyInReach() const;

	void GetReachLine(FVector& outStart, FVector& outEnd) const;
	FAimInfo GetAimInfo() override;

	void LogMsgWithRole(FString message) const;
	FString GetRoleText() const;
	static FString GetEnumText(ENetRole role);
};


