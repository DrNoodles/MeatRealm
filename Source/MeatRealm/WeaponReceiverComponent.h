#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "WeaponReceiverComponent.generated.h"

UENUM()
enum class EWeaponCommands : uint8
{
	FireStart, FireEnd, 
	ReloadStart, ReloadEnd,
};

UENUM()
enum class EWeaponModes : uint8
{
	None = 0, Ready, Firing, Reloading, Paused, ReloadingPaused
};

USTRUCT()
struct FWeaponState
{
	GENERATED_BODY()

	EWeaponModes Mode = EWeaponModes::None;
	int AmmoInClip = 0;
	int AmmoInPool = 0;
	float ReloadProgress = 0; // For Reloading, ReloadingPaused states
	bool HasFired = false; // For Firing state
	bool IsAdsing = false;

	FWeaponState Clone() const
	{
		FWeaponState Clone;
		Clone.Mode = this->Mode;
		Clone.AmmoInClip = this->AmmoInClip;
		Clone.AmmoInPool = this->AmmoInPool;
		Clone.ReloadProgress = this->ReloadProgress;
		Clone.HasFired = this->HasFired;
		Clone.IsAdsing = this->IsAdsing;
		return Clone;
	}
};

USTRUCT()
struct FWeaponInputState
{
	GENERATED_BODY()

	bool FirePressed = false; // 0 nothing, 1, pressed, 2 held, 3 released
	bool AdsPressed = false; // 0 nothing, 1, pressed, 2 held, 3 released
	bool ReloadRequested = false;
	bool HolsterRequested = false;
	bool DrawRequested = false;
};
//
//class UCommandBase : public UObject
//{
//public:
//	
//	UCommandBase()
//	{
//		// TODO Generate time stamp, cmd id, whatnot
//	}
//	virtual ~UCommandBase() = default;
//	virtual FWeaponState Apply(const FWeaponState& InState) const = 0;
//
//private:
//	static unsigned int CommandId;
//};
//
//unsigned UCommandBase::CommandId = 0;
//
//UCLASS()
//class MEATREALM_API UCommandFireStart final : public UCommandBase
//{
//	GENERATED_BODY()
//
//public:
//	FWeaponState Apply(const FWeaponState& InState) const override
//	{
//		// Enforce Ready > Fire
//		if (InState.Mode != EWeaponModes::Ready) return InState;
//
//		FWeaponState OutState = InState.Clone();
//		OutState.Mode = EWeaponModes::Firing;
//		return OutState;
//	}
//};
//
//UCLASS()
//class MEATREALM_API UCommandFireEnd final : public UCommandBase
//{
//	GENERATED_BODY()
//public:
//	FWeaponState Apply(const FWeaponState& InState) const override
//	{
//		// Enforce Firing > Ready
//		if (InState.Mode != EWeaponModes::Firing) return InState;
//
//		FWeaponState OutState = InState.Clone();
//		OutState.Mode = EWeaponModes::Ready;
//		return OutState;
//	}
//};

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
	FWeaponState ChangeState(EWeaponCommands Cmd, const FWeaponState& InState);
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

	
	// All the states!
	void TickReady(float DT);
	void TickFiring(float DT);
	void TickReloading(float DT);

	void SpawnProjectiles() const;
	TArray<FVector> CalcShotPattern() const;

	void AuthHolsterStart();

	bool CanReload() const;
	bool NeedsReload() const;
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

	EWeaponCommands LastCommand;


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
	
	FWeaponState WeaponState{};
	FWeaponInputState InputState{};

	IReceiverComponentDelegate* Delegate = nullptr;

	bool bTriggerPulled;
	int ShotsFired; // Number of shots fired while in the Firing state
	
	/*
	bool bReloadQueued;
	bool bHolsterQueued;
	bool bWasReloadingOnHolster;*/

	FDateTime ClientReloadStartTime;

	bool bIsBusy;
	FTimerHandle BusyTimerHandle;
};
