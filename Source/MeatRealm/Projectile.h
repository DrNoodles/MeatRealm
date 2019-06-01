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
	AProjectile();

	uint32 HeroControllerId;
	void SetHeroControllerId(uint32 HeroContId) { HeroControllerId = HeroContId; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float ShotDamage = 1.0f;

	// Function that initializes the projectile's velocity in the shoot direction.
	void FireInDirection(const FVector& ShootDirection);

	UFUNCTION()
	void OnCompBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* MeshComp = nullptr;

	UPROPERTY(VisibleAnywhere, Category = Projectile)
		USphereComponent* CollisionComp = nullptr;

	UPROPERTY(VisibleAnywhere, Category = Movement)
		UProjectileMovementComponent* ProjectileMovementComp;

};
