// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupBase.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Public/TimerManager.h"
#include "Interfaces/AffectableInterface.h"

// Sets default values
APickupBase::APickupBase()
{
	//PrimaryActorTick.bCanEverTick = true;

	SetReplicates(true);

	CollisionComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionComp"));
	CollisionComp->InitCapsuleSize(15, 20);
	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &APickupBase::OnCompBeginOverlap);
	RootComponent = CollisionComp;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetGenerateOverlapEvents(false);
	MeshComp->SetCollisionProfileName(TEXT("NoCollision"));
	MeshComp->CanCharacterStepUpOn = ECB_No;
}

void APickupBase::BeginPlay()
{
	Super::BeginPlay();
}

void APickupBase::OnCompBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Role == ROLE_Authority) return;

	UE_LOG(LogTemp, Warning, TEXT("Overlapping"));

	const auto IsNotWorthChecking = OtherActor == nullptr || OtherActor == this || OtherComp == nullptr;
	if (IsNotWorthChecking) return;

	auto Affectable = Cast<IAffectableInterface>(OtherActor);
	if (Affectable)
	{
		Affectable->GiveHealth(10);
	}
}

