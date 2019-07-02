#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "WeaponReceiverComponent.generated.h"

UENUM()
enum class EWeaponCommands : uint8
{
	FireStart, FireEnd,
	ReloadStart, ReloadEnd,
	EquipStart, EquipEnd,
	UnEquip,
};

UENUM(BlueprintType)
enum class EWeaponModes : uint8
{
	UnEquipped = 0, Equipping, Idle, Firing, Reloading,
};


USTRUCT(BlueprintType)
struct FWeaponState
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
		EWeaponModes Mode = EWeaponModes::UnEquipped;

	UPROPERTY(BlueprintReadOnly)
		int AmmoInClip = 0;
	
	UPROPERTY(BlueprintReadOnly)
		int AmmoInPool = 0;

	UPROPERTY(BlueprintReadOnly)
		float ReloadProgress = 0; // For Reloading, ReloadingPaused states

	UPROPERTY(BlueprintReadOnly)
		bool IsAdsing = false;

	UPROPERTY(BlueprintReadOnly)
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

	FString ToString() const
	{
		return FString::Printf(TEXT("IsAdsing %s"),(IsAdsing?"T":"F"));
	}
};

USTRUCT()
struct FWeaponInputState
{
	GENERATED_BODY()

	bool FireRequested = false;
	bool AdsRequested = false;
	bool ReloadRequested = false;
	bool HolsterRequested = false;
	bool DrawRequested = false;
	void Reset()
	{
		FireRequested = false;
		AdsRequested = false;
		ReloadRequested = false;
		HolsterRequested = false;
		DrawRequested = false;
	}
};

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
	virtual AActor* GetOwningPawn() = 0;
	virtual FString GetWeaponName() = 0;
	virtual float GetDrawDuration() = 0;
};


inline FString EWeaponCommandsStr(const EWeaponCommands Cmd)
{
	switch (Cmd)
	{
	case EWeaponCommands::FireStart: return "DoFireStart";
	case EWeaponCommands::FireEnd: return "DoFireEnd";
	case EWeaponCommands::ReloadStart: return "DoReloadStart";
	case EWeaponCommands::ReloadEnd: return "DoReloadEnd";
	case EWeaponCommands::EquipStart: return "DoEquipStart";
	case EWeaponCommands::EquipEnd: return "DoEquipEnd";
	case EWeaponCommands::UnEquip: return "DoUnEquip";
	default: return "Unknown";
	}
}
inline FString EWeaponModesStr(const EWeaponModes Mode)
{
	switch (Mode)
	{
	case EWeaponModes::UnEquipped: return "UnEquipped";
	case EWeaponModes::Equipping: return "Equipping";
	case EWeaponModes::Idle: return "Idle";
	case EWeaponModes::Firing: return "Firing";
	case EWeaponModes::Reloading: return "Reloading";
	default: return "Unknown";
	}
}



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MEATREALM_API UWeaponReceiverComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	// Configure the gun

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


protected:

	UPROPERTY(BlueprintReadOnly, Replicated)
		FWeaponState WeaponState {};
	
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

	bool bIsMidReload;
	FDateTime ReloadStartTime;
	FTimerHandle BusyTimerHandle;
	bool bIsBusy;



public:	
	UWeaponReceiverComponent();
	void SetDelegate(IReceiverComponentDelegate* TheDelegate) { Delegate = TheDelegate; }
	void DrawWeapon();
	void HolsterWeapon();
	void PullTrigger();
	void ReleaseTrigger();
	void Reload();
	void AdsPressed();
	void AdsReleased();
	bool TryGiveAmmo();
	bool IsEquipping() const { return WeaponState.Mode == EWeaponModes::Equipping; }

protected:

private:
	bool HasAuthority() const { return GetOwnerRole() == ROLE_Authority; }
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	// All the states!
	bool TickIdle(float DT);
	//bool TickUnEquipped(float DeltaTime);
	bool TickFiring(float DT);
	bool TickReloading(float DT);
	void ReloadEnd();
	void DoTransitionAction(const EWeaponModes OldMode, const EWeaponModes NewMode);
	bool ChangeState(EWeaponCommands Cmd, const FWeaponState& InState);

	TArray<FVector> CalcShotPattern() const;
	bool CanReload() const;
	bool NeedsReload() const;
	void DrawAdsLine(const FColor& Color, float LineLength) const;

	void LogMsgWithRole(FString message);
	FString GetEnumText(ENetRole role);
	AActor* GetGrandpappy() const;
	ENetRole GetOwnerOwnerLocalRole() const;
	ENetRole GetOwnerOwnerRemoteRole() const;
	FString GetRoleText();
};
