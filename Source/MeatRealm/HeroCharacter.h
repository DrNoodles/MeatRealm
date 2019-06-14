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
	float AdsLineLength = 1000; // cm

	UPROPERTY(EditAnywhere)
	float InteractableSearchDistance = 150.f; //cm

	// Projectile class to spawn.
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
		TArray<TSubclassOf<class AWeapon>> WeaponClasses;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float MaxHealth = 100.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float MaxArmour = 100.f;

	UPROPERTY(BlueprintReadOnly, Replicated)
		float Health = 100.f;

	UPROPERTY(BlueprintReadOnly, Replicated)
		float Armour = 0.f;

	UPROPERTY(EditAnywhere)
		float AdsSpeed = 200;

	UPROPERTY(EditAnywhere)
		float WalkSpeed = 400;

	UPROPERTY(BlueprintAssignable, Category = "Event Dispatchers")
		FPlayerTintChanged OnPlayerTintChanged;

protected:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TintChanged)
		FColor TeamTint = FColor::Black;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UStaticMeshComponent* AimPosComp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UArrowComponent* WeaponAnchor = nullptr;

	UPROPERTY(BlueprintReadOnly, Replicated)
		AWeapon* CurrentWeapon = nullptr;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class USceneComponent* FollowCameraOffsetComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* FollowCamera;

	// This is nasty - probably need to work with the official movement states
	bool bIsAdsing = false;
	bool bUseMouseAim = true;
	float AxisMoveUp;
	float AxisMoveRight;
	float AxisFaceUp;
	float AxisFaceRight;

	FVector2D AimPos_ScreenSpace = FVector2D::ZeroVector;
	FVector AimPos_WorldSpace = FVector::ZeroVector;





public:
	AHeroCharacter();
	void Restart() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	void AuthSpawnWeapon(TSubclassOf<AWeapon> weaponClass);

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



	FORCEINLINE AHeroState* GetHeroState() const;
	FORCEINLINE AHeroController* GetHeroController() const;



	/// Input

	void Input_FirePressed() const;
	void Input_FireReleased() const;
	void Input_AdsPressed();
	void Input_AdsReleased();
	void Input_Reload() const;
	void Input_MoveUp(float Value) {	AxisMoveUp = Value; }
	void Input_MoveRight(float Value) { AxisMoveRight = Value; }
	void Input_FaceUp(float Value) { AxisFaceUp = Value; }
	void Input_FaceRight(float Value) { AxisFaceRight = Value; }
	void Input_Interact();

	void SetUseMouseAim(bool bUseMouseAimIn) { bUseMouseAim = bUseMouseAimIn; }

private:

	virtual void Tick(float DeltaSeconds) override;

	static FVector2D GetGameViewportSize();
	static FVector2D CalcLinearLeanVectorUnclipped(const FVector2D& CursorLoc, const FVector2D& ViewportSize);
	void MoveCameraByOffsetVector(const FVector2D& Vector2D, float DeltaSeconds) const;
	FVector2D TrackCameraWithAimMouse() const;
	FVector2D TrackCameraWithAimGamepad() const;
	void ExperimentalMouseAimTracking(float DT);


	void SimulateAdsMode(bool IsAdsing);


	UFUNCTION()
		void OnRep_TintChanged() const;

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_AdsPressed();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_AdsReleased();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_TryInteract();

	template<class T>
	T* ScanForInteractable();

	FHitResult GetFirstPhysicsBodyInReach() const;

	void GetReachLine(FVector& outStart, FVector& outEnd) const;

	void LogMsgWithRole(FString message) const;
	FString GetRoleText() const;
	static FString GetEnumText(ENetRole role);
};


