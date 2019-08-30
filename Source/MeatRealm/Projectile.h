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
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AActor> EffectClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float ShotDamage = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		bool bIsAoe = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		bool bDetonateOnPlayerImpact;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bIsAoe"))
		float FuseTime = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bIsAoe"))
		float InnerRadius = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bIsAoe"))
		float OuterRadius = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bIsAoe"))
		float Falloff = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bIsAoe"))
		bool bDebugVisualiseAoe = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bDebugVisualiseAoe"))
		TSubclassOf<AActor> DebugVisClass;
	
	FTimerHandle DetonationTimerHandle;


	AProjectile();

	void SetInstigatingControllerId(uint32 Id) { InstigatingControllerId = Id; }
	uint32 GetInstigatingControllerId() const { return InstigatingControllerId; }

	float GetCollisionRadius() const;
	float GetGravityZ() const;
	float GetInitialSpeed() const;

	// Function that initializes the projectile's velocity in the shoot direction.
	void SetInitialSpeed(float Speed);

	UFUNCTION()
	void OnCompHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	               FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnCompBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);


private:
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* MeshComp = nullptr;

	UPROPERTY(VisibleAnywhere, Category = Projectile)
		USphereComponent* CollisionComp = nullptr;

	UPROPERTY(VisibleAnywhere, Category = Movement)
		UProjectileMovementComponent* ProjectileMovementComp;

	uint32 InstigatingControllerId;

	void BeginPlay() override;

	void AoeDamage(const FVector& Location);
	void PointDamage(AActor* OtherActor, const FHitResult& Hit);
	void DisableAndDestroy();
	void Detonate();
};
