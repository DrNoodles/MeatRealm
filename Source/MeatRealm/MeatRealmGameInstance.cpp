// Fill out your copyright notice in the Description page of Project Settings.


#include "MeatRealmGameInstance.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

void UMeatRealmGameInstance::Host()
{
	WriteDebugToScreen(FString("Host()"));

	// TODO Test if ServerTravel can execute - currently crashes if instance isn't a listen server

	// Server travel
	UWorld* world = GetWorld();
	if (!ensure(world != nullptr)) { return; }
	world->ServerTravel("/Game/ThirdPersonCPP/Maps/ThirdPersonExampleMap?listen -server");
}

void UMeatRealmGameInstance::Join(const FString& ipaddress)
{
	WriteDebugToScreen("Join: " + ipaddress);

	// Check that ip is in format xxx.xxx.xxx.xxx
	//FRegexPattern pattern{ "\\b\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\b" };
	//FRegexMatcher matcher(pattern, ipaddress);
	//if (!ensure(matcher.FindNext())) { return; }

	// Travel clients to next map
	APlayerController * pc = GetFirstLocalPlayerController();
	if (!ensure(pc != nullptr)) { return; }

	pc->ClientTravel(ipaddress, ETravelType::TRAVEL_Absolute);
}

void UMeatRealmGameInstance::WriteDebugToScreen(FString message, FColor color, float time, int key)
{
	UEngine* gEngine = GetEngine();
	if (!ensure(gEngine != nullptr)) { return; }
	gEngine->AddOnScreenDebugMessage(key, time, color, message);
}
