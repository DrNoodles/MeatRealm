// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/AffectableInterface.h"

#include "HeroCharacter.generated.h"

class AHeroState;
class AHeroController;
class AWeapon;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerTintChanged);

UENUM(BlueprintType)
enum class EWeaponSlots : uint8
{
	Undefined = 0,
	Primary = 1,
	Secondary = 2,
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
		float AdsSpeed = 275;

	UPROPERTY(EditAnywhere)
		float WalkSpeed = 400;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FPlayerTintChanged OnPlayerTintChanged;

	// An angle between 0 and 180 degrees. 
	UPROPERTY(EditAnywhere)
	int BackpedalThresholdAngle = 135;

	UPROPERTY(EditAnywhere)
	float BackpedalSpeedMultiplier = 0.6;

protected:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TintChanged)
		FColor TeamTint = FColor::Black;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UStaticMeshComponent* AimPosComp = nullptr;

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

	// This is nasty - probably need to work with the official movement states
	UPROPERTY(Replicated) // Replicated so we can see enemy aim lines
	bool bIsAdsing = false;

	bool bUseMouseAim = true;
	float AxisMoveUp;
	float AxisMoveRight;
	float AxisFaceUp;
	float AxisFaceRight;

	FVector2D AimPos_ScreenSpace = FVector2D::ZeroVector;
	FVector AimPos_WorldSpace = FVector::ZeroVector;


	UPROPERTY(Replicated)
		EWeaponSlots CurrentWeaponSlot = EWeaponSlots::Undefined;
	UPROPERTY(Replicated)
		AWeapon* Slot1 = nullptr;
	UPROPERTY(Replicated)
		AWeapon* Slot2 = nullptr;



public:
	AHeroCharacter();
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
	bool AuthTryGiveAmmo() override;
	UFUNCTION()
	bool AuthTryGiveArmour(float Delta) override;
	UFUNCTION()
	bool AuthTryGiveWeapon(const TSubclassOf<AWeapon>& Class) override;
	/* End IAffectableInterface */



	AHeroState* GetHeroState() const;
	AHeroController* GetHeroController() const;


	static bool IsBackpedaling(const FVector& MoveDir, const FVector& AimDir, int BackpedalAngle);


	/// Input

	void Input_FirePressed() const;
	void Input_FireReleased() const;
	void Input_AdsPressed();
	void Input_AdsReleased();
	void Input_Reload() const;
	AWeapon* GetWeapon(EWeaponSlots Slot) const;
	void Input_MoveUp(float Value) {	AxisMoveUp = Value; }
	void Input_MoveRight(float Value) { AxisMoveRight = Value; }
	void Input_FaceUp(float Value) { AxisFaceUp = Value; }
	void Input_FaceRight(float Value) { AxisFaceRight = Value; }
	void Input_Interact();
	void Input_PrimaryWeapon();
	void Input_SecondaryWeapon();

	void SetUseMouseAim(bool bUseMouseAimIn) { bUseMouseAim = bUseMouseAimIn; }

	UFUNCTION(BlueprintCallable)
		AWeapon* GetCurrentWeapon() const;
private:

	virtual void Tick(float DeltaSeconds) override;

	
	void SetWeapon(AWeapon* Weapon, EWeaponSlots Slot);
	AWeapon* AuthSpawnAndAttachWeapon(TSubclassOf<AWeapon> weaponClass);
	EWeaponSlots FindGoodSlot() const;
	void AssignWeaponToSlot(AWeapon* Weapon, EWeaponSlots Slot);
	void EquipWeapon(EWeaponSlots Slot);

	static FVector2D GetGameViewportSize();
	static FVector2D CalcLinearLeanVectorUnclipped(const FVector2D& CursorLoc, const FVector2D& ViewportSize);
	void MoveCameraByOffsetVector(const FVector2D& Vector2D, float DeltaSeconds) const;
	FVector2D TrackCameraWithAimMouse() const;
	FVector2D TrackCameraWithAimGamepad() const;
	void ExperimentalMouseAimTracking(float DT);


	void SimulateAdsMode(bool IsAdsing);
	//void DrawAdsLine(const FColor& Color, float LineLength) const;


	UFUNCTION()
		void OnRep_TintChanged() const;

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_AdsPressed();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_AdsReleased();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_TryInteract();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_EquipPrimaryWeapon();
	
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_EquipSecondaryWeapon();


	template<class T>
	T* ScanForInteractable();

	FHitResult GetFirstPhysicsBodyInReach() const;

	void GetReachLine(FVector& outStart, FVector& outEnd) const;

	void LogMsgWithRole(FString message) const;
	FString GetRoleText() const;
	static FString GetEnumText(ENetRole role);
};


