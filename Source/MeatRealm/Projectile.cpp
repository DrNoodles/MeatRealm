// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Controller.h"
#include "PickupBase.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "DrawDebugHelpers.h"

// Sets default values
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = ETickingGroup::TG_PrePhysics;

	SetReplicates(true);
	InitialLifeSpan = 5;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionComp->InitSphereRadius(15.f);
	CollisionComp->OnComponentHit.AddDynamic(this, &AProjectile::OnCompHit);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnCompBeginOverlap);
	CollisionComp->SetCollisionProfileName(FName("Projectile"));

	RootComponent = CollisionComp;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetGenerateOverlapEvents(false);
	MeshComp->SetCollisionProfileName(TEXT("NoCollision"));
	MeshComp->CanCharacterStepUpOn = ECB_No;

	ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComp->SetUpdatedComponent(CollisionComp);
	ProjectileMovementComp->ProjectileGravityScale = 0.0f;
	ProjectileMovementComp->InitialSpeed = 3000.0f;
	ProjectileMovementComp->MaxSpeed = 3000.0f;
	ProjectileMovementComp->bRotationFollowsVelocity = true;
	ProjectileMovementComp->bShouldBounce = false;

	// TODO Show a billboard if by default on the placeholder
}


void AProjectile::InitVelocity(const FVector& ShootDirection)
{
	ProjectileMovementComp->Velocity	= ShootDirection * ProjectileMovementComp->InitialSpeed;
}

void AProjectile::OnCompHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority()) { return; }
	
	//UE_LOG(LogTemp, Warning, TEXT("AProjectile::OnCompHit()"));

	
	// Ignore certain thangs - TODO Use channels tho wholesale solve this problem
	if (OtherActor && (OtherActor->IsA(AProjectile::StaticClass()) || OtherActor->IsA(APickupBase::StaticClass())))
	{
		UE_LOG(LogTemp, Error, TEXT("AProjectile::OnCompHit() - WASTED HIT, OPTIMISE ME OUT WITH CHANNEL"));

		Destroy();

		return;
	}
	
	
	//// If we hit a physics body, nudge it!
	//if (OtherComp && OtherComp->IsSimulatingPhysics())
	//{
	//	const float ImpulseFactor = 10; // TODO Expose as EditAnywhere
	//	OtherComp->AddImpulseAtLocation(ImpulseFactor * GetVelocity(), GetActorLocation());
	//}


	if (bIsAoe) AoeDamage(Hit);
	Destroy();
}


void AProjectile::OnCompBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//UE_LOG(LogTemp, Warning, TEXT("AProjectile::OnCompBeginOverlap()"));

	if (!HasAuthority()) return;

	const auto IsNotWorthChecking = OtherActor == nullptr || OtherActor == Instigator || OtherActor == this || OtherComp == nullptr;
	if (IsNotWorthChecking)
	{
		// ?? Do we need to Destroy() here
		return;
	}


	if (bIsAoe)
	{
		AoeDamage(SweepResult);
	}
	else // Point Damage
	{
		const auto AffectableReceiver = Cast<IAffectableInterface>(OtherActor);
		if (AffectableReceiver == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("AProjectile::OnCompBeginOverlap is overlapping with unsupported type! Optimise me!"));
		}

		PointDamage(OtherActor, SweepResult);
	}

	Destroy();
}

void AProjectile::AoeDamage(const FHitResult& Hit)
{
	if (!GetWorld()) return;

	UE_LOG(LogTemp, Warning, TEXT("AProjectile::AoeDamage()"));

	AController* DmgInstigator = GetInstigatorController();
	//UE_LOG(LogTemp, Warning, TEXT("FoundDmgInstigator: %s"), *FString{ DmgInstigator ? "True" : "False" });

	const bool RadialDmgWasGiven = UGameplayStatics::ApplyRadialDamageWithFalloff(
		GetWorld(), ShotDamage, 0, Hit.Location,
		InnerRadius, OuterRadius, Falloff, UDamageType::StaticClass(),
		{ this }, this, DmgInstigator);// , ECollisionChannel::ECC_Visibility);

	UE_LOG(LogTemp, Warning, TEXT("RadialDmgWasGiven: %s"), *FString{ RadialDmgWasGiven ? "True" : "False" });

	
	// Spawn hit effect
	const FTransform Transform{ Hit.Location };
	FActorSpawnParameters SpawnParameters{};
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* FX = GetWorld()->SpawnActorAbsolute(EffectClass, Transform, SpawnParameters);
	if (!FX)
	{
		UE_LOG(LogTemp, Warning, TEXT("Why?"));
	}
	
	
	if (bDebugVisualiseAoe)
	{
		const auto DebugEffect = UGameplayStatics::BeginDeferredActorSpawnFromClass(
			GetWorld(),
			DebugVisClass,
			Transform,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (DebugEffect != nullptr)
		{
			UGameplayStatics::FinishSpawningActor(DebugEffect, Transform);

			DebugEffect->SetActorScale3D(FVector{ OuterRadius / 50.f });
			/*auto FXComp = Effect->FindComponentByClass<UStaticMeshComponent>();
			if (FXComp)
			{
				FXComp->SetWorldScale3D(FVector{OuterRadius / 50.f});
				UE_LOG(LogTemp, Warning, TEXT("Set Scale asdflkhasdflkjasdhf"));
			}*/
		}

	}
}

void AProjectile::PointDamage(AActor* OtherActor, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("AProjectile::PointDamage()"));
	check(OtherActor);
	if (!OtherActor->bCanBeDamaged) return;

	AActor* Affected = OtherActor->IsA<APawn>() ? (APawn*)OtherActor : OtherActor;
	AController* DmgInstigator = GetInstigatorController();
	//UE_LOG(LogTemp, Warning, TEXT("FoundDmgInstigator: %s"), *FString{ DmgInstigator ? "True" : "False" });

	UGameplayStatics::ApplyPointDamage(Affected, ShotDamage, Hit.ImpactNormal, Hit, DmgInstigator, this, UDamageType::StaticClass());

	// Spawn hit effect
	const FTransform Transform{ Hit.Location };
	FActorSpawnParameters SpawnParameters{};
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	GetWorld()->SpawnActorAbsolute(EffectClass, Transform, SpawnParameters);
}