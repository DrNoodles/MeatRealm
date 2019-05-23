// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "HeroCharacter.h"
#include "GameFramework/Controller.h"

// Sets default values
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	SetReplicates(true);
	//InitialLifeSpan = 10;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionComp->InitSphereRadius(15.f);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnCompBeginOverlap);
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

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::FireInDirection(const FVector& ShootDirection, const FVector& AdditionalVelocity)
{
	ProjectileMovementComp->Velocity
		= (ShootDirection + AdditionalVelocity) * ProjectileMovementComp->InitialSpeed;
}


void AProjectile::OnCompBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//	UE_LOG(LogTemp, Warning, TEXT("BeginOverlap... "));

	if (!HasAuthority()) return;

	const auto IsNotWorthChecking = OtherActor == nullptr || OtherActor == this || OtherComp == nullptr;
	if (IsNotWorthChecking) return;

	// Ignore other projectiles
	if (OtherActor->IsA(AProjectile::StaticClass())) return;

	if (OtherActor->IsA(AHeroCharacter::StaticClass())) // TODO Decouple from HeroCharacter. Introduce IDamageable interface
	{
		// Dont shoot myself
		if (OtherActor == Instigator) return;

		// Damage enemy
		static_cast<AHeroCharacter*>(OtherActor)->ChangeHealth(-ShotDamage);
	}

	Destroy();
}
