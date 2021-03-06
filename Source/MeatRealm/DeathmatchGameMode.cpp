// Fill out your copyright notice in the Description page of Project Settings.

#include "DeathmatchGameMode.h"
#include "DeathmatchGameState.h"
#include "UObject/ConstructorHelpers.h"
#include "HeroCharacter.h"
#include "HeroState.h"
#include "ScoreboardEntryData.h"
#include "HeroController.h"
#include "Engine/PlayerStartPIE.h"
#include "EngineUtils.h"
#include "Structs/DmgHitResult.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "PickupSpawnLocation.h"

ADeathmatchGameMode::ADeathmatchGameMode()
{
	const ConstructorHelpers::FClassFinder<AHeroCharacter> PawnFinder(
		TEXT("/Game/Blueprints/HeroCharacterBP"));
	const ConstructorHelpers::FClassFinder<AHeroController> PlayerControllerFinder(
		TEXT("/Game/Blueprints/HeroControllerBP"));

	DefaultPlayerName = FText::FromString("Sasquatch");
	DefaultPawnClass = PawnFinder.Class;
	PlayerControllerClass = PlayerControllerFinder.Class;
	PlayerStateClass = AHeroState::StaticClass();
	GameStateClass = ADeathmatchGameState::StaticClass();

	bStartPlayersAsSpectators = false;

	PlayerTints.Add(FColor{   0,167,226 });// Sky
	PlayerTints.Add(FColor{ 243,113, 33 });// Orange
	PlayerTints.Add(FColor{  72,173,113 });// Emerald
	PlayerTints.Add(FColor{ 255,207,  1 });// Yellow
	PlayerTints.Add(FColor{ 251,173, 24 });// Light Orange
	PlayerTints.Add(FColor{   0,186,188 });// Turquoise
	PlayerTints.Add(FColor{ 118, 91,167 });// Purple
	PlayerTints.Add(FColor{ 237,  1,127 });// Fuchsia
}


void ADeathmatchGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	const auto Hero = Cast<AHeroController>(NewPlayer);
	ConnectedHeroControllers.Add(Hero->PlayerState->PlayerId, Hero);

	UE_LOG(LogTemp, Warning, TEXT("ConnectedHeroControllers: %d"), ConnectedHeroControllers.Num());
}

void ADeathmatchGameMode::Logout(AController* Exiting)
{
	const auto Hero = Cast<AHeroController>(Exiting);

	ConnectedHeroControllers.Remove(Hero->PlayerState->PlayerId);
	Hero->GetHeroPlayerState()->HasLeftTheGame = true; // not using bIsInactive as it never replicates!

	UE_LOG(LogTemp, Warning, TEXT("ConnectedHeroControllers: %d"), ConnectedHeroControllers.Num());

	Super::Logout(Exiting);
}

bool ADeathmatchGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return false; // Always pick a random spawn
}

void ADeathmatchGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	// Colour the character
	
	auto HChar = Cast<AHeroCharacter>(PlayerPawn);
	if (!HChar) return;

	const auto HCont = HChar->GetHeroController();
	if (!HCont) return;

	int TintNumber;
	const int32 PlayerId = HCont->PlayerState->PlayerId;

	if (PlayerMappedTints.Contains(PlayerId))
	{
		TintNumber = PlayerMappedTints[PlayerId];
	}
	else
	{
		TintNumber = TintCount++;
		PlayerMappedTints.Add(PlayerId, TintNumber);
	}

	HChar->SetTint(PlayerTints[TintNumber]);
}

AActor* ADeathmatchGameMode::FindFurthestPlayerStart(AController* Controller)
{
	//UE_LOG(LogTemp, Warning, TEXT("ADeathmatchGameMode::FindFurthestPlayerStart"));

	UWorld* World = GetWorld();
	APlayerStart* FurthestSpawn = nullptr;
	float FurthestSpawnDistanceToNearestPlayer = 0;

	// O(n*m) Find the spawn furthest from the players

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		APlayerStart* PlayerStart = *It;
		float ClosestEnemyDist = BIG_NUMBER;

		for (const TPair<uint32, AHeroController*>& pair : ConnectedHeroControllers)
		{
			auto HChar = Cast<AHeroCharacter>(pair.Value->GetPawn());
			if (HChar)
			{
				//UE_LOG(LogTemp, Warning, TEXT("GotPawn"));

				float dist = FVector::Dist(PlayerStart->GetActorLocation(), HChar->GetActorLocation());
				if (dist < ClosestEnemyDist)
				{
					ClosestEnemyDist = dist;
				}
			}
		}

		if (ClosestEnemyDist > FurthestSpawnDistanceToNearestPlayer)
		{
			// Found new furthest spawn!
			FurthestSpawn = PlayerStart;
			FurthestSpawnDistanceToNearestPlayer = ClosestEnemyDist;
		}
	}

	return FurthestSpawn;
}

void ADeathmatchGameMode::RestartPlayer(AController* NewPlayer)
{
	// This is copy of GameModeBase's implementation with a change to spawn the furthest player start

	if (NewPlayer == nullptr || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	AActor* StartSpot = FindFurthestPlayerStart(NewPlayer);

	// If a start spot wasn't found,
	if (StartSpot == nullptr)
	{
		// Check for a previously assigned spot
		if (NewPlayer->StartSpot != nullptr)
		{
			StartSpot = NewPlayer->StartSpot.Get();
			UE_LOG(LogGameMode, Warning, TEXT("RestartPlayer: Player start not found, using last start spot"));
		}
	}

	RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
}

void ADeathmatchGameMode::OnPlayerTakeDamage(FMRHitResult Hit)
{
	if (!HasAuthority()) return;

	//UE_LOG(LogTemp, Warning, TEXT("TakeDamage %d"), Hit.DamageTaken);

	if (!ConnectedHeroControllers.Contains(Hit.ReceiverControllerId))
	{
		UE_LOG(LogTemp, Error, TEXT("ADeathmatchGameMode::OnPlayerTakeDamage cant find receiver controller!"));
		return;
	}
	if (!ConnectedHeroControllers.Contains(Hit.AttackerControllerId))
	{
		UE_LOG(LogTemp, Error, TEXT("ADeathmatchGameMode::OnPlayerTakeDamage cant find attacker controller!"));
		return;
	}

	const auto AttackerController = ConnectedHeroControllers[Hit.AttackerControllerId];
	// TODO Route hit location through OnPlayerTakeDamage. Probs time to introduce a hit struct!
	
	AttackerController->SimulateHitGiven(Hit);


	const auto ReceivingController = ConnectedHeroControllers[Hit.ReceiverControllerId];
	

	if (Hit.HealthRemaining <= 0)
	{
		// Award the killer a point
		if (AttackerController) AttackerController->GetPlayerState<AHeroState>()->Kills++;
		
		// Award death point, kill then respawn character
		if (ReceivingController)
		{
			ReceivingController->GetPlayerState<AHeroState>()->Deaths++;

			AHeroCharacter* DeadChar = ReceivingController->GetHeroCharacter();
			if (DeadChar)
			{
				DeadChar->SpawnHeldWeaponsAsPickups();
				DeadChar->Destroy();
			}

			RestartPlayer(ReceivingController);
		}

		AddKillfeedEntry(AttackerController, ReceivingController);
	}
}


bool ADeathmatchGameMode::ReadyToStartMatch_Implementation()
{
	// If bDelayed Start is set, wait for a manual match start
	if (bDelayedStart)
	{
		return false;
	}

	// By default start when we have > 0 players
	if (GetMatchState() == MatchState::WaitingToStart)
	{
		if (NumPlayers + NumBots > 0)
		{
			return true;
		}
	}
	return false;
}


bool ADeathmatchGameMode::ReadyToEndMatch_Implementation()
{
	// TODO OPTIMISE THIS! It's called on every tick and GetScoreboard is slow.

	auto DMGameState = GetGameState<ADeathmatchGameState>();
	auto Scores = DMGameState->GetScoreboard();
	
	const auto bFragLimitReached = Scores.Num() > 0 && Scores[0]->Kills >= DMGameState->FragLimit;
	return bFragLimitReached;
}

void ADeathmatchGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	UE_LOG(LogTemp, Warning, TEXT("ADeathmatchGameMode::HandleMatchHasStarted()"));


	GetWorldTimerManager().SetTimer(ChestAnnouncementTimerHandle, this, &ADeathmatchGameMode::AnnounceChestSpawn, PowerUpSpawnRate, true, PowerUpInitialDelay);
}

void ADeathmatchGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	/*if (GetWorld()) */GetWorld()->GetTimerManager().ClearTimer(ChestAnnouncementTimerHandle);

	// TODO Disable shooting
	// TODO Show scoreboards on clients

	// TODO Delay 5 seconds
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("No world found in HandleMatchHasEnded?!"));
		RestartGame();
		return;
	}



	// WriteDebugToScreen
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5, FColor::White, "WINNER WINNER, TURDUCKEN DINNER", true, FVector2D{ 2,2 });
	}


	FTimerHandle CanActionTimerHandle;

	World->GetTimerManager().SetTimer(
		CanActionTimerHandle, this, &ADeathmatchGameMode::OnRestartGame,5, false, -1);
}
void ADeathmatchGameMode::OnRestartGame()
{
	//	World->ServerTravel("/Game/MeatRealm/Maps/TestMap");
	
	RestartGame();
}

void ADeathmatchGameMode::AddKillfeedEntry(AHeroController* const Killer, AHeroController* const Dead)
{
	FString KillerName{}, DeadName{};

	if (Killer) KillerName = Killer->PlayerState->GetPlayerName(); // will this crash if they've left?
	if (Dead) DeadName = Dead->PlayerState->GetPlayerName();

	auto*const GS = GetGameState<ADeathmatchGameState>();
	if (GS)
	{
		GS->AddKillfeedData(KillerName, "killed", DeadName);
	}
}







// Weapon Drops //////////////////////////////////////////////////////////

void ADeathmatchGameMode::AnnounceChestSpawn()
{
	UWorld* World = GetWorld();
	if (!World) return;



	// Find the next spawn location
	float min = TNumericLimits< float >::Min();
	float max = TNumericLimits< float >::Max();
	float MinX = max, MinY = max, MaxX = min, MaxY = min;
	TArray<APickupSpawnLocation*> Spawns{};
	for (TActorIterator<APickupSpawnLocation> It(World); It; ++It)
	{
		APickupSpawnLocation* TP = *It;
		if (TP->ActorHasTag(FName("ChestSpawnLocation")))
		{
			if (TP->PickupClass == nullptr)
			{
				UE_LOG(LogTemp, Error, TEXT("Set the pickup spawn class to spawn in a derived Blueprint"));
				continue;
			}

			Spawns.Emplace(TP);

			// Find bounds of all possible points - TODO this could be done once per map if we want to optimise
			auto L = TP->GetActorLocation();
			if (L.X < MinX) MinX = L.X;
			if (L.X > MaxX) MaxX = L.X;
			if (L.Y < MinY) MinY = L.Y;
			if (L.Y > MaxY) MaxY = L.Y;
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("Weapon Pickups Found: %d"), Spawns.Num());

	if (Spawns.Num() == 0)
	{
		return;
	}

	const auto Choice = FMath::RandRange(0, Spawns.Num() - 1);
	NextChestSpawnLocation = Spawns[Choice];



	// Spawn drop placeholder

	const auto PreviewClass = NextChestSpawnLocation->PreviewClass;
	if (PreviewClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Set the pickup spawn class to spawn in a derived Blueprint"));
	}
	else
	{
		FActorSpawnParameters Params{};
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		auto* Preview = GetWorld()->SpawnActor<AActor>(PreviewClass, NextChestSpawnLocation->GetActorTransform(), Params);
		if (Preview)
		{
			Preview->SetLifeSpan(PowerUpAnnouncementLeadTime);
		}
	}



	// Find WHERE it will spawn and notify approx. location

	const auto ActorLocation = NextChestSpawnLocation->GetActorLocation();

	UE_LOG(LogTemp, Warning, TEXT("x:%f X:%f y:%f Y:%f Loc: %s"), MinX, MaxX, MinY, MaxY, *ActorLocation.ToString());

	// Find which map section the item is spawning
	const FString LeftRightStr = (ActorLocation.Y < (MaxY + MinY) / 2) ? "Left" : "Right";
	const FString TopBottomStr = (ActorLocation.X < (MaxX + MinX) / 2) ? "Bottom" : "Top";
	const FString LocationMsg = FString::Printf(TEXT("%s %s"), *TopBottomStr, *LeftRightStr);

	// Notify incoming chest! Use delegate so the hud can bind onto it
	auto* const GS = GetGameState<ADeathmatchGameState>();
	if (GS) GS->NotifyIncomingSuper(PowerUpAnnouncementLeadTime, LocationMsg);



	// Set timer to spawn the thing

	GetWorldTimerManager().SetTimer(ChestSpawnTimerHandle, this, &ADeathmatchGameMode::SpawnChest, PowerUpAnnouncementLeadTime);
}
void ADeathmatchGameMode::SpawnChest()
{
	if (!NextChestSpawnLocation)
		return;

	FActorSpawnParameters Params{};
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;


	auto* Pickup = GetWorld()->SpawnActorAbsolute<APickupBase>(NextChestSpawnLocation->PickupClass, NextChestSpawnLocation->GetActorTransform(), Params);

	GetWorldTimerManager().ClearTimer(ChestSpawnTimerHandle);
}
