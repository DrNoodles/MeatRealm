#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "WeaponReceiverComponent.generated.h"

class IReceiverComponentDelegate
{
public:
	virtual ~IReceiverComponentDelegate() = default;
	virtual void ShotFired() = 0;
	virtual void AmmoInClipChanged(int AmmoInClip) = 0;
	virtual void AmmoInPoolChanged(int AmmoInPool) = 0;
	virtual void InReloadingChanged(bool IsReloading) = 0;
	virtual void OnReloadProgressChanged(float ReloadProgress) = 0;
	virtual bool SpawnAProjectile(const FVector& Direction) = 0;
	virtual FVector GetBarrelDirection() = 0;
	virtual FVector GetBarrelLocation() = 0;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MEATREALM_API UWeaponReceiverComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UWeaponReceiverComponent();
	void SetDelegate(IReceiverComponentDelegate* TheDelegate) { Delegate = TheDelegate; }
	void Draw();
	void QueueHolster();
	void Input_PullTrigger();
	void Input_ReleaseTrigger();
	void Input_Reload();
	void Input_AdsPressed();
	void Input_AdsReleased();
	bool TryGiveAmmo();

protected:
	UFUNCTION()
		void OnRep_IsReloadingChanged();

private:
	bool HasAuthority() const { return GetOwnerRole() == ROLE_Authority; }
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void RemoteTick(float DeltaTime);
	void AuthTick(float DeltaTime);


	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_Reload();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_PullTrigger();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_ReleaseTrigger();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_AdsPressed();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerRPC_AdsReleased();


	void SpawnProjectiles() const;

	TArray<FVector> CalcShotPattern() const;

	void AuthFireStart();
	void AuthFireEnd();
	void AuthHolsterStart();
	void AuthReloadStart();
	void AuthReloadEnd();

	bool CanReload() const;
	bool NeedsReload() const;
	bool IsMatchInProgress() const;
	void DrawAdsLine(const FColor& Color, float LineLength) const;

	void LogMsgWithRole(FString message);
	FString GetEnumText(ENetRole role);
	ENetRole GetOwnerOwnerLocalRole() const;
	ENetRole GetOwnerOwnerRemoteRole() const;
	FString GetRoleText();


public:

	// Configure the gun

//// Time (seconds) to holster the weapon
//	UPROPERTY(EditAnywhere)
//		float HolsterDuration = 1;

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

	/*UPROPERTY(EditAnywhere)
		float AdsMovementScale = 0.70;*/


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
	UPROPERTY(EditAnywhere)
		float AdsLineLength = 1500; // cm

	UPROPERTY(EditAnywhere)
		FColor AdsLineColor = FColor{ 255,0,0 };

	UPROPERTY(EditAnywhere)
		float EnemyAdsLineLength = 175; // cm

	UPROPERTY(EditAnywhere)
		FColor EnemyAdsLineColor = FColor{ 255,170,75 };

	UPROPERTY(Replicated)
		bool bAdsPressed;


	IReceiverComponentDelegate* Delegate = nullptr;

	bool bCanAction;
	bool bTriggerPulled;
	bool bHasActionedThisTriggerPull;
	bool bReloadQueued;
	bool bHolsterQueued;
	bool bWasReloadingOnHolster;
	FDateTime ClientReloadStartTime;
	FTimerHandle CanActionTimerHandle;

};
