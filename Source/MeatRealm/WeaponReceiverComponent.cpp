#include "WeaponReceiverComponent.h"
#include "UnrealNetwork.h"

void UWeaponReceiverComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWeaponReceiverComponent, AmmoInClip);
	DOREPLIFETIME(UWeaponReceiverComponent, AmmoInPool);
	DOREPLIFETIME(UWeaponReceiverComponent, bIsReloading);
}

UWeaponReceiverComponent::UWeaponReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}
void UWeaponReceiverComponent::BeginPlay()
{
	Super::BeginPlay();
}
void UWeaponReceiverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UWeaponReceiverComponent::OnRep_IsReloadingChanged()
{
}

