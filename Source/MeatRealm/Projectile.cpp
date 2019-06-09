// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Controller.h"
#include "PickupBase.h"


// Sets default values
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	SetReplicates(true);
	InitialLifeSpan = 5;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionComp->InitSphereRadius(15.f);
	CollisionComp->OnComponentHit.AddDynamic(this, &AProjectile::OnCompHit);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnCompBeginOverlap);

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionComp->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

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


void AProjectile::FireInDirection(const FVector& ShootDirection)
{
	ProjectileMovementComp->Velocity	= ShootDirection * ProjectileMovementComp->InitialSpeed;
}

void AProjectile::OnCompHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("AProjectile::OnCompHit()"));

	if (!HasAuthority()) return;

	// If we hit a physics body, nudge it!
	if (OtherComp && OtherComp->IsSimulatingPhysics())
	{
		const float ImpulseFactor = 10; // TODO Expose as EditAnywhere
		OtherComp->AddImpulseAtLocation(ImpulseFactor * GetVelocity(), GetActorLocation());
	}

	Destroy();
}


void AProjectile::OnCompBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("AProjectile::OnCompBeginOverlap()"));

	if (!HasAuthority()) return;

	const auto TheReceiver = OtherActor;

	const auto IsNotWorthChecking = TheReceiver == nullptr || TheReceiver == this || OtherComp == nullptr;
	if (IsNotWorthChecking) return;

	// Ignore other projectiles
	if (TheReceiver->IsA(AProjectile::StaticClass())) return;
	if (TheReceiver->IsA(APickupBase::StaticClass())) return;

	if (TheReceiver->GetClass()->ImplementsInterface(UAffectableInterface::StaticClass()))
	{
		// Dont shoot myself
		if (TheReceiver == Instigator) return;

		// Apply damage
		auto AffectableReceiver = Cast<IAffectableInterface>(TheReceiver);
		if (AffectableReceiver == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("AffectableReceiver of damage is null!"));
		}
		AffectableReceiver->ApplyDamage(HeroControllerId, ShotDamage, GetActorLocation());
	}

	Destroy();
}
