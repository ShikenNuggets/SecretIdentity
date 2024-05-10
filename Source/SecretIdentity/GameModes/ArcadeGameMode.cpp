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
		ACrisisSpawnPoint* CSP = Cast<ACrisisSpawnPoint>(A);
		WARN_IF_NULL(CSP);
		if (CSP != nullptr)
		{
			CrisisSpawnPoints.Add(CSP);
			CSP->OnCrisisResolved.AddUObject(this, &AArcadeGameMode::OnCrisisResolved);
		}
	}

	LOG_MSG("Found " + FString::FromInt(CrisisSpawnPoints.Num()) + " CrisisSpawnPoint");

	//Reset Time/Timers
	fCurrentSpawnTime = StartSpawnTime;
	fTimer = fCurrentSpawnTime - 5.0f; //We want the first crisis to spawn very quickly

	LOG_MSG("The next crisis will spawn in " + FString::SanitizeFloat(FMath::RoundHalfFromZero(fCurrentSpawnTime - fTimer), 0) + " seconds");

	WARN_IF_NULL(ThugEnemyClass);

	if (ThugEnemyClass != nullptr)
	{
		ThugEnemyClass->GetDefaultObject(true); //Create the default object upfront so it's ready to be spawned in later
	}
}

void AArcadeGameMode::Tick(float DeltaTime)
{
	fTimer += DeltaTime;
	if (fTimer >= fCurrentSpawnTime)
	{
		SpawnCrisis();
		fTimer -= fCurrentSpawnTime;

		fCurrentSpawnTime -= 1.0f; //Enemies spawn in faster and faster as time goes on
		fCurrentSpawnTime = FMath::Clamp(fCurrentSpawnTime, 1.0f, std::numeric_limits<float>::infinity()); //Things get weird if this number gets too low

		LOG_MSG("The next crisis will spawn in " + FString::SanitizeFloat(FMath::RoundHalfFromZero(fCurrentSpawnTime - fTimer), 0) + " seconds");
	}

	fCurrentFearPercentage = GetFearPercentage();
	OnUpdateFearMeter.Broadcast(static_cast<float>(fCurrentFearPercentage)); //This is mainly for UI so we're okay with lower precision

	if (fCurrentFearPercentage >= 1.0f)
	{
		GameOver();
	}

	OnUpdateSessionTimer.Broadcast(UGameplayStatics::GetTimeSeconds(this));
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
	else
	{

		OnUpdateCrisisCount.Broadcast(GetNumActiveCrises());
	}
}

int AArcadeGameMode::GetNumActiveCrises()
{
	int ActiveCrises = 0;

	for (const auto& CSP : CrisisSpawnPoints)
	{
		if (CSP->IsCrisisActiveAndNotResolved())
		{
			ActiveCrises++;
		}
	}

	return ActiveCrises;
}

double AArcadeGameMode::GetTotalFear()
{
	double FearTotal = 0.0f;

	for (const auto& CSP : CrisisSpawnPoints)
	{
		double FearAmount = CSP->GetTimeSinceCrisisStarted() / 5.0f; //Every 5 seconds a crisis is active, it increases our fear total by one
		WARN_IF(FearAmount < 0.0f);
		FearTotal += FMath::Clamp(FearAmount, 0.0, std::numeric_limits<double>::infinity());
	}

	return FearTotal;
}

double AArcadeGameMode::GetFearPercentage()
{
	double FearPercentage = GetTotalFear() / (CrisisSpawnPoints.Num() * 60.0f);
	WARN_IF(FearPercentage < 0.0f); //Greater than 1 is okay, but this should DEFINITELY never be negative
	return FMath::Clamp(FearPercentage, 0.0f, 1.0f); 
}

void AArcadeGameMode::OnCrisisResolved(ACrisisSpawnPoint* CSP)
{
	LOG_MSG("Crisis resolved!");
	OnUpdateCrisisCount.Broadcast(GetNumActiveCrises());
	OnUpdateFearMeter.Broadcast(GetFearPercentage());
}

void AArcadeGameMode::GameOver()
{
	GetWorld()->GetWorldSettings()->SetTimeDilation(0.0f);
	OnUpdateSessionTimer.Broadcast(UGameplayStatics::GetTimeSeconds(this));
	OnGameOver.Broadcast();
}