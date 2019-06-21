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

inline FString EWeaponCommandsStr(const EWeaponCommands Cmd)
{
	switch (Cmd) 
	{ 
		case EWeaponCommands::FireStart: return "FireStart";
		case EWeaponCommands::FireEnd: return "FireEnd";
		case EWeaponCommands::ReloadStart: return "ReloadStart";
		case EWeaponCommands::ReloadEnd: return "ReloadEnd";
		default: return "Unknown";
	}
}


UENUM(BlueprintType)
enum class EWeaponModes : uint8
{
	None = 0, Ready, Firing, Reloading, Paused, ReloadingPaused
};


inline FString EWeaponModesStr(const EWeaponModes Mode)
{
	switch (Mode)
	{
	case EWeaponModes::None: return "None";
	case EWeaponModes::Ready: return "Ready";
	case EWeaponModes::Firing: return "Firing";
	case EWeaponModes::Reloading: return "Reloading";
	case EWeaponModes::Paused: return "Paused";
	case EWeaponModes::ReloadingPaused: return "ReloadingPaused";
	default: return "Unknown";
	}
}


USTRUCT(BlueprintType)
struct FWeaponState
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
		EWeaponModes Mode = EWeaponModes::None;

	UPROPERTY(BlueprintReadOnly)
		int AmmoInClip = 0;
	
	UPROPERTY(BlueprintReadOnly)
		int AmmoInPool = 0;

	UPROPERTY(BlueprintReadOnly)
		float ReloadProgress = 0; // For Reloading, ReloadingPaused states

	UPROPERTY(BlueprintReadOnly)
		bool IsAdsing = false;

	bool HasFired = false; // For Firing state

	FWeaponState Clone() const
	{
		FWeaponState Clone;
		Clone.Mode = this->Mode;
		Clone.AmmoInClip = this->AmmoInClip;
		Clone.AmmoInPool = this->AmmoInPool;
		Clone.ReloadProgress = this->ReloadProgress;
		Clone.IsAdsing = this->IsAdsing;
		Clone.HasFired = this->HasFired;
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
	//UFUNCTION()
	//	void OnRep_IsReloadingChanged();

private:
	bool HasAuthority() const { return GetOwnerRole() == ROLE_Authority; }
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	FWeaponState ChangeState(EWeaponCommands Cmd, const FWeaponState& InState);


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

	

	EWeaponCommands LastCommand;

	UPROPERTY(Replicated, BlueprintReadOnly)
		FWeaponState WeaponState {};

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



	FWeaponInputState InputState{};

	IReceiverComponentDelegate* Delegate = nullptr;

	int ShotsFired; // Number of shots fired while in the Firing state
	
	/*
	bool bReloadQueued;
	bool bHolsterQueued;
	bool bWasReloadingOnHolster;*/

	bool bIsMidReload;
	FDateTime ReloadStartTime;

	bool bIsBusy;
	FTimerHandle BusyTimerHandle;
};
