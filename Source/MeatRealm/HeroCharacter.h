// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Weapon.h"
#include "Interfaces/AffectableInterface.h"

#include "HeroCharacter.generated.h"

class AHeroState;
class AHeroController;


// TODO Use something built in already?
USTRUCT(BlueprintType)
struct MEATREALM_API FMRHitResult
{
	GENERATED_BODY()

	UPROPERTY()
		uint32 ReceiverControllerId;
	UPROPERTY()
		uint32 AttackerControllerId;
	UPROPERTY()
		int HealthRemaining;
	UPROPERTY()
		int DamageTaken;
	UPROPERTY()
		bool bHitArmour;
	UPROPERTY()
		FVector HitLocation;
	UPROPERTY()
		FVector HitDirection;
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
		TArray<TSubclassOf<class AWeapon>> WeaponClasses;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseLookUpRate;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Replicated)
		float Health = 100.f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Replicated)
		float Armour = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float MaxHealth = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float MaxArmour = 100.f;



	UFUNCTION()
	virtual void ApplyDamage(uint32 InstigatorHeroControllerId, float Damage, FVector Location) override;
	UFUNCTION()
	virtual bool TryGiveHealth(float Hp) override;
	UFUNCTION()
	virtual bool TryGiveAmmo() override;
	UFUNCTION()
	virtual bool TryGiveArmour(float Delta) override;
	UFUNCTION()
	virtual bool TryGiveWeapon(const TSubclassOf<AWeapon>& Class) override;



	// Sets default values for this character's properties
	AHeroCharacter();
	void Restart() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	AHeroState* GetHeroState() const;
	AHeroController* GetHeroController() const;



	/// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UArrowComponent* WeaponAnchor = nullptr;

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_SpawnWeapon(TSubclassOf<AWeapon> weaponClass);

	UPROPERTY(BlueprintReadOnly, Replicated)
		AWeapon* CurrentWeapon = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UStaticMeshComponent* AimPosComp = nullptr;


	/// Input

	void Input_FirePressed() const { if (CurrentWeapon) CurrentWeapon->Input_PullTrigger(); }
	void Input_FireReleased() const { if (CurrentWeapon) CurrentWeapon->Input_ReleaseTrigger(); }
	void Input_Reload() const { if (CurrentWeapon) CurrentWeapon->Input_Reload(); }
	void Input_MoveUp(float Value) {	AxisMoveUp = Value; }
	void Input_MoveRight(float Value) { AxisMoveRight = Value; }
	void Input_FaceUp(float Value) { AxisFaceUp = Value; }
	void Input_FaceRight(float Value) { AxisFaceRight = Value; }
	void Input_Interact();

	void SetUseMouseAim(bool bUseMouseAimIn) { bUseMouseAim = bUseMouseAimIn; }


protected:
	static FVector2D GetGameViewportSize();
	static FVector2D CalcLinearLeanVectorUnclipped(const FVector2D& CursorLoc, const FVector2D& ViewportSize);
	void MoveCameraByOffsetVector(const FVector2D& Vector2D, float DeltaSeconds) const;
	bool IsClientControllingServerOwnedActor();
	virtual void Tick(float DeltaSeconds) override;
	FVector2D TrackCameraWithAimMouse() const;
	FVector2D TrackCameraWithAimGamepad() const;
	void ExperimentalMouseAimTracking(float DT);


private:

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class USceneComponent* FollowCameraOffsetComp;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* FollowCamera;

	bool bUseMouseAim = true;
	float AxisMoveUp;
	float AxisMoveRight;
	float AxisFaceUp;
	float AxisFaceRight;


	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_TryInteract();

	template<class T>
	T* ScanForInteractable();
	FHitResult GetFirstPhysicsBodyInReach() const;
	void GetReachLine(FVector& outStart, FVector& outEnd) const;

	void LogMsgWithRole(FString message) const;
	FString GetEnumText(ENetRole role) const;
	FString GetRoleText() const;

	FVector2D AimPos_ScreenSpace = FVector2D::ZeroVector;
	FVector AimPos_WorldSpace = FVector::ZeroVector;
};


