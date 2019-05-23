// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Weapon.h"

#include "HeroCharacter.generated.h"

UCLASS()
class MEATREALM_API AHeroCharacter : public ACharacter
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

	//bool Method(AActor* Owner, APawn* Instigator, AController* InstigatorController, AController* Controller);
	virtual void BeginPlay() override;

	// Projectile class to spawn.
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
		TArray<TSubclassOf<class AWeapon>> WeaponClasses;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseLookUpRate;


	UPROPERTY(BlueprintReadOnly) // TODO Move to PlayerController (or PlayerState?)
		int Deaths;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
		float Health = 100.f;

	UFUNCTION(BlueprintCallable)
		void ChangeHealth(float delta);


protected:
	// AActor interface
	virtual void Tick(float DeltaSeconds) override;
	// End of AActor interface

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }


	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_SpawnWeapon(TSubclassOf<AWeapon> weaponClass);

	UPROPERTY(ReplicatedUsing = OnRep_ServerStateChanged)
		AWeapon* ServerCurrentWeapon;

	UFUNCTION()
		void OnRep_ServerStateChanged();


	UPROPERTY(BlueprintReadOnly)
		AWeapon* CurrentWeapon = nullptr;

private:
	/// Input

	void Input_FirePressed();
	void Input_FireReleased();
	void Input_Reload();

	/// Components


	UPROPERTY(VisibleAnywhere)
		UArrowComponent* WeaponAnchor = nullptr;
};
