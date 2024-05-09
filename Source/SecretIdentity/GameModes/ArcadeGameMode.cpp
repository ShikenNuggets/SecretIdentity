// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "ArcadeGameMode.h"

#include "Kismet/GameplayStatics.h"

#include "SecretIdentity/UE_Helpers.h"
#include "SecretIdentity/Actors/CrisisSpawnPoint.h"

AArcadeGameMode::AArcadeGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AArcadeGameMode::BeginPlay()
{
	LOG_MSG(TEXT("Arcade Game Mode - BeginPlay"));

	//Get all the Crisis Spawn Points
	CrisisSpawnPoints.Empty();

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACrisisSpawnPoint::StaticClass(), FoundActors);

	for (const auto& A : FoundActors)
	{
		ACrisisSpawnPoint* csp = Cast<ACrisisSpawnPoint>(A);
		WARN_IF_NULL(csp);
		if (csp != nullptr)
		{
			CrisisSpawnPoints.Add(csp);
		}
	}

	LOG_MSG("Found " + FString::FromInt(CrisisSpawnPoints.Num()) + " CrisisSpawnPoint");

	//Reset Time/Timers
	fCurrentSpawnTime = StartSpawnTime;
	fTimer = fCurrentSpawnTime - 5.0f; //We want the first crisis to spawn very quickly

	LOG_MSG("The next crisis will spawn in " + FString::SanitizeFloat(fCurrentSpawnTime - fTimer) + " seconds");

	WARN_IF_NULL(ThugEnemyClass);
}

void AArcadeGameMode::Tick(float DeltaTime)
{
	fTimer += DeltaTime;
	if (fTimer >= fCurrentSpawnTime)
	{
		SpawnCrisis();
		fTimer -= fCurrentSpawnTime;

		LOG_MSG("The next crisis will spawn in " + FString::SanitizeFloat(fCurrentSpawnTime - fTimer) + " seconds");
	}
}

void AArcadeGameMode::SpawnCrisis()
{
	bool spawnedCrisis = false;

	//Limit the number of iterations of this loop so it doesn't deadlock the whole game if we get really unlucky
	//TODO - Toy with this number to see if we can get better results with acceptable performance
	//Higher number gives us a better sense of randomness when lots of crises are active
	//Lower number improves performance but will reduce the randomness
	
	//TODO 2 - This is adequate but we should probably do something slightly more intelligent
	//For example, consider the position of the player, or try to guarantee that crises aren't too close together
	for (int i = 0; i < 50; i++)
	{
		int32 fRandomNumber = FMath::RandRange(0, CrisisSpawnPoints.Num() - 1);
		WARN_IF(fRandomNumber >= CrisisSpawnPoints.Num());
		if (fRandomNumber >= CrisisSpawnPoints.Num())
		{
			fRandomNumber = 0; //Just in case we get an invalid number (I do not expect this to ever be an issue, but who knows...)
		}

		if (CrisisSpawnPoints[fRandomNumber]->IsCrisisActive())
		{
			continue; //Try to find a different one
		}

		LOG_MSG(TEXT("Found crisis to spawn randomly"));
		CrisisSpawnPoints[fRandomNumber]->SpawnCrisis(ThugEnemyClass);
		spawnedCrisis = true;
		break;
	}

	//If we couldn't find one randomly, just loop through the list sequentially
	if (!spawnedCrisis)
	{
		for (const auto& CSP : CrisisSpawnPoints)
		{
			if (CSP->IsCrisisActive())
			{
				continue;
			}

			LOG_MSG(TEXT("Found crisis to spawn sequentially"));
			CSP->SpawnCrisis(ThugEnemyClass);
			spawnedCrisis = true;
			break;
		}
	}

	//If all spawn points are already active, we'll just skip this cycle
	if (!spawnedCrisis)
	{
		LOG_MSG(TEXT("All crisis spawn points were active, skipping this cycle"));
	}
}