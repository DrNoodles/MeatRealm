// Fill out your copyright notice in the Description page of Project Settings.


#include "MeatRealmGameInstance.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Engine/ObjectLibrary.h"
#include "DeathmatchGameMode.h"
#include "FileManager.h"
#include "Paths.h"

void UMeatRealmGameInstance::Host(const FString& MapName)
{
	//WriteDebugToScreen(FString("Host: " + MapName));

	const auto World = GetWorld();
	if (World) World->ServerTravel("/Game/MeatRealm/Maps/TestMap?", true);

	// TODO Put a big fucking bit of text explaining what's happening on screen
	// TODO Change after a timer has expired (3 seconds?)
}

void UMeatRealmGameInstance::ServerChangeMap_Implementation()
{
	const auto World = GetWorld();
	//if (World) World->ServerTravel(TEXT("/Game/MeatRealm/Maps/TestMap"));
}

bool UMeatRealmGameInstance::ServerChangeMap_Validate()
{
	return true;
}

void UMeatRealmGameInstance::ListMaps()
{
	auto Maps = GetAllMapNames2();
	WriteDebugToScreen(FString("Maps:"));

	for (FString& m : Maps)
	{
		WriteDebugToScreen(FString("  "+m));
	}

}

void UMeatRealmGameInstance::Join(const FString& ipaddress)
{
	WriteDebugToScreen("Join: " + ipaddress);

	// Travel client to next map
	APlayerController * pc = GetFirstLocalPlayerController();
	if (!ensure(pc != nullptr)) { return; }

	pc->ClientTravel(ipaddress, ETravelType::TRAVEL_Absolute);
}

void UMeatRealmGameInstance::WriteDebugToScreen(FString message, FColor color, float time, int key) const
{
	UEngine* gEngine = GetEngine();
	if (!ensure(gEngine != nullptr)) { return; }
	gEngine->AddOnScreenDebugMessage(key, time, color, message);
}

TArray<FString> UMeatRealmGameInstance::GetAllMapNames()
{
	auto ObjectLibrary = UObjectLibrary::CreateLibrary(UWorld::StaticClass(), false, true);
	ObjectLibrary->LoadAssetDataFromPath(TEXT("/Game/MeatRealm/Maps/"));
	TArray<FAssetData> AssetDatas;
	ObjectLibrary->GetAssetDataList(OUT AssetDatas);

	UE_LOG(LogTemp, Warning, TEXT("Found maps: count:%d got:%d"), ObjectLibrary->GetAssetDataCount(), AssetDatas.Num());

	TArray<FString> Names = TArray<FString>();

	for (FAssetData& AssetData : AssetDatas)
	{
		auto name = AssetData.AssetName.ToString();
		Names.Add(name);
	}

	return Names;
}

UFUNCTION(BlueprintPure, meta = (DisplayName = "GetAllMapNames", Keywords = "GAMNMaps"), Category = "MapNames")
FORCEINLINE TArray<FString> UMeatRealmGameInstance::GetAllMapNames2()
{
	TArray<FString> MapFiles;

	IFileManager::Get().FindFilesRecursive(MapFiles, *FPaths::ProjectContentDir(), TEXT("*.umap"), true, false, false);

	for (int32 i = 0; i < MapFiles.Num(); i++)
	{
		//replace the whole directory string with only the name of the map

		int32 lastSlashIndex = -1;
		if (MapFiles[i].FindLastChar('/', lastSlashIndex))
		{
			FString pureMapName;

			//length - 5 because of the ".umap" suffix
			for (int32 j = lastSlashIndex + 1; j < MapFiles[i].Len() - 5; j++)
			{
				pureMapName.AppendChar(MapFiles[i][j]);
			}

			MapFiles[i] = pureMapName;
		}
	}

	return MapFiles;
}