// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/AffectableInterface.h"

#include "HeroCharacter.generated.h"

class AHeroState;
class AHeroController;
class AWeapon;
class IEquippable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerTintChanged);

UENUM(BlueprintType)
enum class EInventorySlots : uint8
{
	Undefined = 0,
	Primary = 1,
	Secondary = 2,
	Health = 3,
};

UCLASS()
class MEATREALM_API AHeroCharacter : public ACharacter, public IAffectableInterface
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

	// Projectile class to spawn.
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
		TArray<TSubclassOf<class AWeapon>> DefaultWeaponClass;

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
		float RunTurnRateBase = 45;

	UPROPERTY(EditAnywhere)
		float RunTurnRateMax = 270;

	// Seconds until an action works after running 
	UPROPERTY(EditAnywhere)
		float RunCooldown = 0.5;

	UPROPERTY(EditAnywhere)
	bool bCancelReloadOnRun = true;

	// Not replicated cuz diff local vs server time;
	FDateTime LastRunEnded;

	FTimerHandle RunEndTimerHandle;

	UPROPERTY(EditAnywhere)
	float Deadzone = 0.3;


	UPROPERTY(EditAnywhere, Category = Debug)
		bool bDrawMovementInput;

	UPROPERTY(EditAnywhere, Category = Debug)
		bool bDrawMovementVector;

	UPROPERTY(EditAnywhere, Category = Debug)
		bool bDrawMovementSpeed;


protected:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TintChanged)
		FColor TeamTint = FColor::Black;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UStaticMeshComponent* AimPosComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UArrowComponent* WeaponAnchor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UArrowComponent* HolsteredweaponAnchor = nullptr;

	UPROPERTY(Replicated, BlueprintReadOnly)
		EInventorySlots CurrentInventorySlot = EInventorySlots::Undefined;

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

	FTimerHandle EquipTimerHandle;

	bool bIsEquipping;

	bool bWantsToFire;


	UPROPERTY(Transient, Replicated)
	bool bIsRunning = false;

	UPROPERTY(Transient, Replicated)
	bool bIsTargeting = false;
	
	const char* HandSocketName = "HandSocket";
	const char* Holster1SocketName = "Holster1Socket";
	const char* Holster2SocketName = "Holster2Socket";


	UPROPERTY(Replicated)
		EInventorySlots LastInventorySlot = EInventorySlots::Undefined;

	
	UPROPERTY(Replicated)
		AWeapon* PrimaryWeaponSlot = nullptr;
	UPROPERTY(Replicated)
		AWeapon* SecondaryWeaponSlot = nullptr;
	UPROPERTY(Replicated)
		AItemBase* HealthSlot = nullptr;



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


	/* IAffectableInterface */
	UFUNCTION()
	void AuthApplyDamage(uint32 InstigatorHeroControllerId, float Damage, FVector Location) override;
	UFUNCTION()
	bool AuthTryGiveHealth(float Hp) override;
	UFUNCTION()
	bool CanGiveAmmo() override;
	UFUNCTION()
	bool AuthTryGiveAmmo() override;
	UFUNCTION()
	bool AuthTryGiveArmour(float Delta) override;
	UFUNCTION()
	bool AuthTryGiveWeapon(const TSubclassOf<AWeapon>& Class) override;
	UFUNCTION()
	bool CanGiveWeapon(const TSubclassOf<AWeapon>& Class, float& OutDelay) override;
	UFUNCTION()
	bool CanGiveItem(const TSubclassOf<AItemBase>& Class, float& OutDelay) override;
	UFUNCTION()
	bool TryGiveItem(const TSubclassOf<AItemBase>& Class) override;
	/* End IAffectableInterface */


	FTransform GetAimTransform() const;

	AHeroState* GetHeroState() const;
	AHeroController* GetHeroController() const;
	float GetTargetingSpeedModifier() const;
	bool IsReloading() const;
	


	static bool IsBackpedaling(const FVector& MoveDir, const FVector& AimDir, int BackpedalAngle);


	/// Input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	void UseItemStart();
	void UseItemStop();


	void StartWeaponFire();
	void StopWeaponFire();
	bool IsFiring() const;

	void Input_FirePressed();
	void Input_FireReleased();
	void Input_AdsPressed();
	void Input_AdsReleased();
	void Input_Reload() const;

	UFUNCTION(BlueprintCallable)
	AWeapon* GetWeapon(EInventorySlots Slot) const;
	AItemBase* GetItem(EInventorySlots Slot) const;
	IEquippable* GetEquippable(EInventorySlots Slot) const;

	void Input_MoveUp(float Value) {	AxisMoveUp = Value; }
	void Input_MoveRight(float Value) { AxisMoveRight = Value; }
	void Input_FaceUp(float Value) { AxisFaceUp = Value; }
	void Input_FaceRight(float Value) { AxisFaceRight = Value; }
	void Input_Interact();
	void Input_PrimaryWeapon();
	void Input_SecondaryWeapon();
	void Input_ToggleWeapon();

	void SetUseMouseAim(bool bUseMouseAimIn) { bUseMouseAim = bUseMouseAimIn; }



	UFUNCTION(BlueprintCallable)
		int GetHealthItemCount() const;

	UFUNCTION(BlueprintCallable)
	AWeapon* GetCurrentWeapon() const;

	UFUNCTION(BlueprintCallable)
	AItemBase* GetCurrentItem() const;

	IEquippable* GetCurrentEquippable() const;

	bool IsRunning() const { return bIsRunning; }
	bool IsTargeting() const;
	float GetRunningSpeed() const { return RunningSpeed; }
	
	float GetRunningReloadSpeed() const { return RunningReloadSpeed; }

private:

	AWeapon* FindWeaponToReceiveAmmo() const;

	void ScanForWeaponPickups(float DeltaSeconds);
	virtual void Tick(float DeltaSeconds) override;
	void TickWalking(float DT);
	void TickRunning(float DT);

	void OnEquipHealth();
	void EquipHealth();
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerEquipHealth();

	void OnRunToggle();
	void OnStartRunning();
	void OnStopRunning();
	void SetRunning(bool bNewIsRunning);


	void SetTargeting(bool bNewTargeting);

	void GiveItemToPlayer(TSubclassOf<class AItemBase> ItemClass);


	void GiveWeaponToPlayer(TSubclassOf<class AWeapon> WeaponClass);
	AWeapon* AuthSpawnWeapon(TSubclassOf<AWeapon> weaponClass);
	EInventorySlots FindGoodWeaponSlot() const;
	AWeapon* AssignWeaponToInventorySlot(AWeapon* Weapon, EInventorySlots Slot);
	void EquipSlot(EInventorySlots Slot);
	void MakeEquippedItemVisible() const;
	void RefreshWeaponAttachments() const;

	static FVector2D GetGameViewportSize();
	static FVector2D CalcLinearLeanVectorUnclipped(const FVector2D& CursorLoc, const FVector2D& ViewportSize);
	void MoveCameraByOffsetVector(const FVector2D& Vector2D, float DeltaSeconds) const;
	FVector2D TrackCameraWithAimMouse() const;
	FVector2D TrackCameraWithAimGamepad() const;
	void ExperimentalMouseAimTracking(float DT);




	UFUNCTION(Server, Reliable, WithValidation)
		void ServerSetTargeting(bool bNewTargeting);

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerSetRunning(bool bNewWantsToRun);

	UFUNCTION()
		void OnRep_TintChanged() const;

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_TryInteract();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_EquipPrimaryWeapon();
	
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_EquipSecondaryWeapon();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_ToggleWeapon();


	template<class T>
	T* ScanForInteractable();

	FHitResult GetFirstPhysicsBodyInReach() const;

	void GetReachLine(FVector& outStart, FVector& outEnd) const;

	void LogMsgWithRole(FString message) const;
	FString GetRoleText() const;
	static FString GetEnumText(ENetRole role);
};


