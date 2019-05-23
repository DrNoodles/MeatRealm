// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "HeroCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/Public/DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "UnrealNetwork.h"
#include "HeroState.h"
#include "HeroController.h"



/// Lifecycle

AHeroCharacter::AHeroCharacter()
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

	// Replication
	bAlwaysRelevant = true;

	JumpMaxCount = 0;

	// Configure character movement
	GetCharacterMovement()->MaxWalkSpeed = 450;
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character move independently of facing
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bAbsoluteRotation = true; // Don't rotate with the character
	CameraBoom->RelativeRotation = FRotator(-56.f, 0.f, 0.f);
	CameraBoom->TargetArmLength = 1500;
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level
	CameraBoom->bEnableCameraLag = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bAbsoluteRotation = false;
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
	FollowCamera->SetFieldOfView(67);

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	WeaponAnchor = CreateDefaultSubobject<UArrowComponent>(TEXT("WeaponAnchor"));
	WeaponAnchor->SetupAttachment(RootComponent);
}


void AHeroCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHeroCharacter, ServerCurrentWeapon);
}


//
//bool AHeroCharacter::Method(AActor* Owner, APawn* Instigator, AController* InstigatorController, AController* Controller)
//{
//	FString a = Owner ? "Actor" : "";
//	FString b = Instigator ? "Instigator" : "";
//	FString c = InstigatorController ? " InstigatorController" : "";
//	FString d = Controller ? " Controller" : "";
//	UE_LOG(LogTemp, Warning, TEXT("%s %s %s %s"), *a, *b, *c, *d);
//
//	return Owner || Instigator || InstigatorController || Controller;
//}

AHeroState* AHeroCharacter::GetHeroState() const
{
	return (AHeroState*)GetPlayerState();
}

AHeroController* AHeroCharacter::GetHeroController() const
{
	return (AHeroController*)GetController();
}

void AHeroCharacter::BeginPlay()
{
	Super::BeginPlay();



	/*auto owner = GetOwner();
	auto instigator = Instigator;
	auto instigatorController = GetInstigatorController();
	auto controller = GetController();
	auto b = Method(owner, instigator, instigatorController, controller);*/

	// Randomly select a weapon
	if (WeaponClasses.Num() > 0)
	{
		if (ROLE_AutonomousProxy == Role)
		{
			const auto Choice = FMath::RandRange(0, WeaponClasses.Num() - 1);
			ServerRPC_SpawnWeapon(WeaponClasses[Choice]);
		}
	}
}

void AHeroCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveUp");
	PlayerInputComponent->BindAxis("MoveRight");
	PlayerInputComponent->BindAxis("FaceUp");
	PlayerInputComponent->BindAxis("FaceRight");
	PlayerInputComponent->BindAction(
		"FireWeapon", IE_Pressed, this, &AHeroCharacter::Input_FirePressed);
	PlayerInputComponent->BindAction(
		"FireWeapon", IE_Released, this, &AHeroCharacter::Input_FireReleased);
	PlayerInputComponent->BindAction(
		"Reload", IE_Released, this, &AHeroCharacter::Input_Reload);
}

void AHeroCharacter::ServerRPC_SpawnWeapon_Implementation(TSubclassOf<AWeapon> weaponClass)
{
	FActorSpawnParameters params;
	params.Instigator = this;
	params.Owner = this;

	auto weapon = GetWorld()->SpawnActorAbsolute<AWeapon>(
		weaponClass,
		WeaponAnchor->GetComponentTransform(), params);

	weapon->AttachToComponent(
		WeaponAnchor, FAttachmentTransformRules{ EAttachmentRule::KeepWorld, true });

	ServerCurrentWeapon = weapon;
}

bool AHeroCharacter::ServerRPC_SpawnWeapon_Validate(TSubclassOf<AWeapon> weaponClass)
{
	return true;
}

void AHeroCharacter::OnRep_ServerStateChanged()
{
	// TODO Destroy other current weapons?

	// TODO Branch for ROLE_Auto, ROLE_Sim

	CurrentWeapon = ServerCurrentWeapon;
}

void AHeroCharacter::Input_FirePressed()
{
	if (CurrentWeapon == nullptr) return;
	CurrentWeapon->Input_PullTrigger();
}

void AHeroCharacter::Input_FireReleased()
{
	if (CurrentWeapon == nullptr) return;
	CurrentWeapon->Input_ReleaseTrigger();
}

void AHeroCharacter::Input_Reload()
{
	if (CurrentWeapon == nullptr) return;
	CurrentWeapon->Input_Reload();
}
/// Methods

void AHeroCharacter::Tick(float DeltaSeconds)
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

void AHeroCharacter::ApplyDamage(AHeroCharacter* DamageInstigator, float Damage)
{
	// TODO Only on authority, then rep player state to all clients.
	Health -= Damage;
	bIsDead = Health <= 0;

	if (bIsDead)
	{
		HealthDepletedEvent.Broadcast(this, DamageInstigator);

		// TODO Destroy weapon
		// TODO Destroy character
	}

	//// Report health to the screen
	//if (true||HasAuthority())
	//{
	//	APlayerState* PlayerState = GetPlayerState();
	//	FString PlayerName = PlayerState->GetPlayerName();

	//	if (GEngine)
	//	{
	//		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, 
	//			FString::Printf(TEXT("%s: %fhp"), *PlayerName, Health));
	//	}
	//}
}

