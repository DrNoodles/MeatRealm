// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

UCLASS()
class MEATREALM_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* MeshComp = nullptr;

	UPROPERTY(EditAnywhere , Category = Projectile)
		USphereComponent* CollisionComp = nullptr;

	UPROPERTY(VisibleAnywhere, Category = Movement)
		UProjectileMovementComponent* ProjectileMovementComp;

	UPROPERTY(EditAnywhere)
		float ShotsPerSecond = 1.0f;

	UPROPERTY(EditAnywhere)
		float ShotDamage = 1.0f;

	UPROPERTY(EditAnywhere)
		bool bRepeats = true;

	// Function that initializes the projectile's velocity in the shoot direction.
	void FireInDirection(const FVector& ShootDirection, const FVector& AdditionalVelocity);
};
