// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UnrealNetwork.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Info.h"

#include "KillfeedEntryData.generated.h"


//class MEATREALM_API UNetworkObject : public UObject
//{
//	GENERATED_BODY()
//
//protected:
//	virtual bool IsSupportedForNetworking() const override;
//	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags);
//};
//

UCLASS(BlueprintType)
class MEATREALM_API UKillfeedEntryData : public UActorComponent
{
	GENERATED_BODY()

public:
	
	virtual bool IsSupportedForNetworking() const override
	{
		return true;
	}
	//virtual Replica
	//virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
	//{
	//	return true;
	//}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Fart)
		int TotalKills = 0;

	UFUNCTION()
		void OnRep_Fart();
	/*
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
		FString Winner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
		FString Verb;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated)
		FString Loser;*/
};
