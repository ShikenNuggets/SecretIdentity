// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "ArcadeGameMode.h"

#include "GameFramework/PlayerStart.h"
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
	Super::BeginPlay();

	aPlayerStart = UGameplayStatics::GetActorOfClass(this, APlayerStart::StaticClass());

	//Just a debug check
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
	WARN_IF_MSG(PlayerStarts.Num() <= 0, "Level has no PlayerStarts");
	WARN_IF_MSG(PlayerStarts.Num() > 1, "Level has multiple PlayerStarts");

	eCurrentState = StartState;
	WARN_IF(eCurrentState >= EArcadeGameState::Count);
	switch (eCurrentState)
	{
		case EArcadeGameState::Menu:
			StartMenuState();
			break;
		case EArcadeGameState::Play:
			StartPlayState();
			break;
		case EArcadeGameState::Count: //Intentional fallthrough
		default:
			break;
	}

	WARN_IF_NULL(aPlayerStart);
}

void AArcadeGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (eCurrentState != EArcadeGameState::Play)
	{
		return;
	}

	fTimer += DeltaTime;
	if (fTimer >= fCurrentSpawnTime)
	{
		SpawnCrisis();
		fTimer -= fCurrentSpawnTime;

		fCurrentSpawnTime -= 1.0f; //Enemies spawn in faster and faster as time goes on
		fCurrentSpawnTime = FMath::Clamp(fCurrentSpawnTime, 1.0f, std::numeric_limits<float>::infinity()); //Things get weird if this number gets too low
	}

	fCurrentFearPercentage = GetFearPercentage();
	OnUpdateFearMeter.Broadcast(static_cast<float>(fCurrentFearPercentage)); //This is mainly for UI so we're okay with lower precision

	if (fCurrentFearPercentage >= 1.0f)
	{
		GameOver();
	}

	OnUpdateSessionTimer.Broadcast(UGameplayStatics::GetTimeSeconds(this));
}

void AArcadeGameMode::StartMenuState()
{
	LOG_MSG("Start Menu State");
	SpawnPawn(MenuPawnBP, 2'000);
	OnStartMenuState.Broadcast(CurrentPawn);
}

void AArcadeGameMode::StartPlayState()
{
	LOG_MSG("Start Play State");
	SpawnPawn(PlayPawnBP);

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

	WARN_IF(CrisisSpawnPoints.IsEmpty());

	//Reset Time/Timers
	fCurrentSpawnTime = StartSpawnTime;
	fTimer = fCurrentSpawnTime - 5.0f; //We want the first crisis to spawn very quickly

	WARN_IF_NULL(ThugEnemyClass);

	if (ThugEnemyClass != nullptr)
	{
		ThugEnemyClass->GetDefaultObject(true); //Create the default object upfront so it's ready to be spawned in later
	}

	OnStartPlayState.Broadcast(CurrentPawn);
}

void AArcadeGameMode::SpawnCrisis()
{
	bool spawnedCrisis = false;
	if (CrisisSpawnPoints.IsEmpty())
	{
		return;
	}

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

			CSP->SpawnCrisis(ThugEnemyClass);
			spawnedCrisis = true;
			break;
		}
	}

	//If all spawn points are already active, we'll just skip this cycle
	if (spawnedCrisis)
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
	if (CrisisSpawnPoints.IsEmpty())
	{
		return 0.0;
	}

	double FearPercentage = GetTotalFear() / (CrisisSpawnPoints.Num() * 60.0f);
	WARN_IF(FearPercentage < 0.0f); //Greater than 1 is okay, but this should DEFINITELY never be negative
	return FMath::Clamp(FearPercentage, 0.0f, 1.0f); 
}

void AArcadeGameMode::OnCrisisResolved(ACrisisSpawnPoint* CSP)
{
	OnUpdateCrisisCount.Broadcast(GetNumActiveCrises());
	OnUpdateFearMeter.Broadcast(GetFearPercentage());
}

void AArcadeGameMode::GameOver()
{
	GetWorld()->GetWorldSettings()->SetTimeDilation(0.0f);
	OnUpdateSessionTimer.Broadcast(UGameplayStatics::GetTimeSeconds(this));
	OnGameOver.Broadcast();
}

void AArcadeGameMode::SpawnPawn(TSubclassOf<APawn> PawnToSpawn, double ZOffset)
{
	/*if (CurrentPawn != nullptr)
	{
		CurrentPawn->Destroy();
		CurrentPawn = nullptr;
	}

	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	if (aPlayerStart != nullptr)
	{
		Location = aPlayerStart->GetActorLocation() + FVector(0.0f, 0.0f, ZOffset);
		Rotation = aPlayerStart->GetActorRotation();
	}

	LOG_MSG("Spawning pawn at " + Location.ToString());

	CurrentPawn = Cast<APawn>(GetWorld()->SpawnActor(PawnToSpawn));
	WARN_IF_NULL(CurrentPawn);*/
}