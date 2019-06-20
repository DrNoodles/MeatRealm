#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "WeaponReceiverComponent.generated.h"

class IReceiverComponentDelegate
{
public:
	virtual ~IReceiverComponentDelegate() = default;
private:
	virtual void AmmoInClipChanged(int AmmoInClip) = 0;
	virtual void AmmoInPoolChanged(int AmmoInPool) = 0;
	virtual void InReloadingChanged(bool IsReloading) = 0;
	virtual void OnReloadProgressChanged(float ReloadProgress) = 0;
};



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MEATREALM_API UWeaponReceiverComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UWeaponReceiverComponent();
	void SetDelegate(IReceiverComponentDelegate* TheDelegate) { Delegate = TheDelegate; }

protected:

private:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	UFUNCTION()
		void OnRep_IsReloadingChanged();

public:

	// Configure the gun

// Time (seconds) to holster the weapon
	UPROPERTY(EditAnywhere)
		float HolsterDuration = 1;

	UPROPERTY(EditAnywhere)
		float ShotsPerSecond = 1.0f;

	UPROPERTY(EditAnywhere)
		bool bFullAuto = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float AdsSpread = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float HipfireSpread = 20;

	UPROPERTY(EditAnywhere)
		int AmmoPoolSize = 30;

	UPROPERTY(EditAnywhere)
		int AmmoPoolGiven = 20;

	UPROPERTY(EditAnywhere)
		int AmmoGivenPerPickup = 10;

	UPROPERTY(EditAnywhere)
		bool bUseClip = true;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bUseClip"))
		int ClipSizeGiven = 0;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bUseClip"))
		int ClipSize = 10;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "bUseClip"))
		float ReloadTime = 3;

	// The number of projectiles fired per shot
	UPROPERTY(EditAnywhere)
		int ProjectilesPerShot = 1;

	// When ProjectilesPerShot > 1 this ensures all projectiles are spread evenly across the HipfireSpread angle.
	UPROPERTY(EditAnywhere)
		bool bEvenSpread = true;

	// This makes even spreading feel more natural by randomly clumping the shots within the even spread.
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bEvenSpread"))
		bool bSpreadClumping = true;

	UPROPERTY(EditAnywhere)
		float AdsMovementScale = 0.70;


	// Gun status

	UPROPERTY(BlueprintReadOnly, Replicated)
		int AmmoInClip;

	UPROPERTY(BlueprintReadOnly, Replicated)
		int AmmoInPool;

	// Used for UI binding
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsReloadingChanged)
		bool bIsReloading = false;
	
	// Used for UI binding
	UPROPERTY(BlueprintReadOnly)
		float ReloadProgress = 0.f;



protected:

private:

	//OnAmmoInClipChanged;
	//OnAmmoInPoolChanged;
	//OnIsReloadingChanged(bool)
	//OnReloadProgressChanged(float)
	//FShotFired OnShotFired;

	IReceiverComponentDelegate* Delegate = nullptr;
};
