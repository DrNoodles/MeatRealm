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

private:

	//UPROPERTY(VisibleAnywhere)
	//	USceneComponent* Root = nullptr;

	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* Mesh = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
		USphereComponent* Collision = nullptr;

	UPROPERTY(VisibleAnywhere, Category = Movement)
		UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
		float ShotsPerSecond = 1.0f;

	UPROPERTY(EditAnywhere)
		float ShotDamage = 1.0f;

	UPROPERTY(EditAnywhere)
		bool bRepeats = true;
};
