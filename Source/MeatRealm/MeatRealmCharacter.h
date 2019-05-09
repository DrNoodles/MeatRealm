// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MeatRealmCharacter.generated.h"

UCLASS(config=Game)
class AMeatRealmCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

public:
	AMeatRealmCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;


	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "MeatRealm Character")
	bool isDead = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeatRealm Character")
	float Health = 100.f;


	UFUNCTION(BlueprintCallable, Category = "MeatRealm Character")
	void ChangeHealth(float delta);

	//void CalculateDead();


protected:
	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Handler for when a touch input begins. */
	void OnTouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void OnTouchStopped(ETouchIndex::Type FingerIndex, FVector Location);


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
};

