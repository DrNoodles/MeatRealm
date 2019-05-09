// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "MeatRealmCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"


/// Lifecycle

AMeatRealmCharacter::AMeatRealmCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character move independently of facing
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bAbsoluteRotation = true; // Don't rotate with the character
	CameraBoom->RelativeRotation = FRotator(-70.f, 0.f, 0.f);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level
	CameraBoom->bEnableCameraLag = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bAbsoluteRotation = false;
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

void AMeatRealmCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveUp");
	PlayerInputComponent->BindAxis("MoveRight");
	PlayerInputComponent->BindAxis("FaceUp");
	PlayerInputComponent->BindAxis("FaceRight");

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AMeatRealmCharacter::OnTouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AMeatRealmCharacter::OnTouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AMeatRealmCharacter::OnResetVR);
}


/// Methods

void AMeatRealmCharacter::Tick(float DeltaSeconds)
{
	// Handle Input
	if (Controller == nullptr) return;

	const auto deadzoneSquared = 0.25f * 0.25f;

	// Move character
	const auto moveVec = FVector{ GetInputAxisValue("MoveUp"), GetInputAxisValue("MoveRight"), 0 };
	if (moveVec.SizeSquared() >= deadzoneSquared)
	{
		AddMovementInput(FVector{ 1.f, 0.f, 0.f }, moveVec.X);
		AddMovementInput(FVector{ 0.f, 1.f, 0.f }, moveVec.Y);
	}

	// Aim character with look, if look is below deadzone then try use move vec
	const auto lookVec = FVector{ GetInputAxisValue("FaceUp"), GetInputAxisValue("FaceRight"), 0 };
	if (lookVec.SizeSquared() >= deadzoneSquared)
	{
		Controller->SetControlRotation(lookVec.Rotation());
	}
	else if (moveVec.SizeSquared() >= deadzoneSquared)
	{
		Controller->SetControlRotation(moveVec.Rotation());
	}
}

void AMeatRealmCharacter::ChangeHealth(float delta)
{
	Health += delta;
	isDead = Health <= 0;
}
//
//void AMeatRealmCharacter::CalculateDead()
//{
//	
//}



/// Event Handlers

void AMeatRealmCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AMeatRealmCharacter::OnTouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AMeatRealmCharacter::OnTouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}
