// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MeatRealmGameInstance.generated.h"

UCLASS()
class MEATREALM_API UMeatRealmGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(Exec)
	void Host(const FString& MapName);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerChangeMap();

	UFUNCTION(Exec)
		void ListMaps();

	UFUNCTION(Exec)
	void Join(const FString& ipaddress);

private:

	void WriteDebugToScreen(FString message, FColor color = FColor::Blue, 
		float time = 5.f,
		int key = -1) const;

	static TArray<FString> GetAllMapNames();
	static TArray<FString> GetAllMapNames2();
};
