// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Weapon.h"
#include "Interfaces/AffectableInterface.h"

#include "HeroCharacter.generated.h"

class AHeroState;
class AHeroController;

UCLASS()
class MEATREALM_API AHeroCharacter : public ACharacter, public IAffectableInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* FollowCamera;


public:


	// Sets default values for this character's properties
	AHeroCharacter();
	void Restart() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	AHeroState* GetHeroState() const;
	AHeroController* GetHeroController() const;


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
	virtual void ApplyDamage(uint32 InstigatorHeroControllerId, float Damage) override;
	UFUNCTION()
	virtual bool TryGiveHealth(float Hp) override;
	UFUNCTION()
	virtual bool TryGiveAmmo() override;
	UFUNCTION()
	virtual bool TryGiveArmour(float Delta) override;
	UFUNCTION()
	virtual bool TryGiveWeapon(const TSubclassOf<AWeapon>& Class) override;

protected:
	virtual void Tick(float DeltaSeconds) override;


public:

	/// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		UArrowComponent* WeaponAnchor = nullptr;



	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_SpawnWeapon(TSubclassOf<AWeapon> weaponClass);

	UPROPERTY(BlueprintReadOnly, Replicated)
		AWeapon* CurrentWeapon = nullptr;


	/// Input

	void Input_FirePressed() const { if (CurrentWeapon) CurrentWeapon->Input_PullTrigger(); }
	void Input_FireReleased() const { if (CurrentWeapon) CurrentWeapon->Input_ReleaseTrigger(); }
	void Input_Reload() const { if (CurrentWeapon) CurrentWeapon->Input_Reload(); }
	void Input_MoveUp(float Value) {	AxisMoveUp = Value; }
	void Input_MoveRight(float Value) { AxisMoveRight = Value; }
	void Input_FaceUp(float Value) { AxisFaceUp = Value; }
	void Input_FaceRight(float Value) { AxisFaceRight = Value; }

	void SetUseMouseAim(bool bUseMouse) { bUseMouseAim = bUseMouse; }


private:
	bool bUseMouseAim = true;
	float AxisMoveUp;
	float AxisMoveRight;
	float AxisFaceUp;
	float AxisFaceRight;
	  
	void LogMsgWithRole(FString message);
	FString GetEnumText(ENetRole role);
	FString GetRoleText();
};
