// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "ArcadeGameMode.h"

#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

#include "SecretIdentity/UE_Helpers.h"
#include "SecretIdentity/Actors/ArcadePlayerStart.h"
#include "SecretIdentity/Actors/CrisisSpawnPoint.h"

AArcadeGameMode::AArcadeGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AArcadeGameMode::BeginPlay()
{
	Super::BeginPlay();

#if UE_BUILD_SHIPPING
	DebugFastFearMeter = false;
	DebugDisableFearMeter = false;
	DebugFastSpawnCrises = false;
	DebugSpawnAllCrises = false;
#endif

	if (DebugFastFearMeter || DebugDisableFearMeter || DebugFastSpawnCrises || DebugSpawnAllCrises)
	{
		LOG_MSG_WARNING("Debug values are enabled");
	}

	aPlayerStart = Cast<AArcadePlayerStart>(UGameplayStatics::GetActorOfClass(this, AArcadePlayerStart::StaticClass()));

	//Just a debug check
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(this, AArcadePlayerStart::StaticClass(), PlayerStarts);
	WARN_IF_MSG(PlayerStarts.IsEmpty(), "Level has no ArcadePlayerStarts");
	WARN_IF_MSG(PlayerStarts.Num() > 1, "Level has multiple ArcadePlayerStarts");

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
			WARN_IF_MSG(true, "EArcadeGameState case not handled!");
			break;
	}

	WARN_IF_NULL(GetWorld());
	WARN_IF_NULL(aPlayerStart);
}

void AArcadeGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (eCurrentState != EArcadeGameState::Play)
	{
		fTimer = 0.0f;
		fPlayStateTimer = 0.0f;
		return;
	}

	fTimer += DeltaTime;
	fPlayStateTimer += DeltaTime;

	if (fNumCrisesResolved < 3 && IsAnyCrisisActive())
	{
		fTimer = 0.0f; //Only allow one crisis at a time to be resolved for the first 3
	}

	float SpawnTime = fCurrentSpawnTime;
	if (!IsAnyCrisisActive() && SpawnTime > 5.0f)
	{
		SpawnTime = 5.0f;
	}
	else if (GetNumActiveCrises() * 5.0f < SpawnTime)
	{
		SpawnTime = GetNumActiveCrises() * 5.0f; //If one is spawned, wait 5 seconds. If two are spawned, wait 10. Etc until fCurrentSpawnTime
	}

#if !UE_BUILD_SHIPPING
	if (DebugFastSpawnCrises)
	{
		fTimer = fCurrentSpawnTime;
	}
#endif //!UE_BUILD_SHIPPING

	if (fTimer >= SpawnTime)
	{
		SpawnCrisis();
		fTimer -= SpawnTime;

		fCurrentSpawnTime -= 1.0f; //Enemies spawn in faster and faster as time goes on
		fCurrentSpawnTime = FMath::Clamp(fCurrentSpawnTime, 1.0f, std::numeric_limits<float>::infinity()); //Things get weird if this number gets too low
	}

	fCurrentFearPercentage = GetFearPercentage();
	OnUpdateFearMeter.Broadcast(static_cast<float>(fCurrentFearPercentage)); //This is mainly for UI so we're okay with lower precision

	if (fCurrentFearPercentage >= 1.0f)
	{
		GameOver();
	}

	OnUpdateSessionTimer.Broadcast(fPlayStateTimer);
}

void AArcadeGameMode::StartMenuState()
{
	eCurrentState = EArcadeGameState::Menu;

	LOG_MSG("Start Menu State");
	SpawnPlayerPawn(PlayPawnBP);
	OnStartMenuState.Broadcast(CurrentPawn);
}

void AArcadeGameMode::StartPlayState()
{
	WARN_IF_NULL(GetWorld());
	if (GetWorld() == nullptr)
	{
		return; //Can't set the timer :(
	}

	OnBeginTransitionToPlayState.Broadcast(aPlayStatePawn, PlayStateTransitionTime);

	FTimerHandle fTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(fTimerHandle, FTimerDelegate::CreateLambda([&]
	{
		eCurrentState = EArcadeGameState::Play;

		LOG_MSG("Start Play State");

		//Get all the Crisis Spawn Points
		CrisisSpawnPoints.Empty();

		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACrisisSpawnPoint::StaticClass(), FoundActors);
		WARN_IF(FoundActors.IsEmpty());

		for (const auto& A : FoundActors)
		{
			ACrisisSpawnPoint* CSP = Cast<ACrisisSpawnPoint>(A);
			WARN_IF_NULL(CSP);
			if (CSP != nullptr)
			{
				CrisisSpawnPoints.Add(CSP);
				OnStartPlayState.AddDynamic(CSP, &ACrisisSpawnPoint::OnPlayStateBegins);
				CSP->OnCrisisResolved.AddUObject(this, &AArcadeGameMode::OnCrisisResolved);
			}
		}

		WARN_IF(CrisisSpawnPoints.IsEmpty());

		//Reset Time/Timers
		fCurrentSpawnTime = StartSpawnTime;

		WARN_IF_NULL(ThugEnemyClass);

		if (ThugEnemyClass != nullptr)
		{
			ThugEnemyClass->GetDefaultObject(true); //Create the default object upfront so it's ready to be spawned in later
		}

		OnStartPlayState.Broadcast(aPlayStatePawn);
	}), PlayStateTransitionTime, false);
}

bool AArcadeGameMode::IsAnyCrisisActive()
{
	return GetNumActiveCrises() > 0;
}

FVector AArcadeGameMode::GetNearestActiveCrisisLocationToPlayer()
{
	if (!IsAnyCrisisActive())
	{
		return FVector::Zero();
	}

	ACrisisSpawnPoint* NearestPoint = nullptr;
	double NearestDistance = std::numeric_limits<double>::infinity();

	for (const auto& CSP : CrisisSpawnPoints)
	{
		WARN_IF_NULL(CSP);
		if (CSP != nullptr && CSP->IsCrisisActiveAndNotResolved())
		{
			double Distance = FMath::Abs(FVector::Distance(CSP->GetAverageCrisisActorLocation(), aPlayStatePawn->GetActorLocation()));
			if (Distance < NearestDistance || NearestPoint == nullptr)
			{
				NearestPoint = CSP;
				NearestDistance = Distance;
			}
		}
	}

	if (NearestPoint != nullptr)
	{
		return NearestPoint->GetAverageCrisisActorLocation() + FVector(0.0f, 0.0f, 45.0f);
	}

	return FVector::Zero();
}

void AArcadeGameMode::RequestPauseOrUnpause()
{
	if (eCurrentState != EArcadeGameState::Play || bGameOver)
	{
		return;
	}

	bIsPaused = !bIsPaused;
	float NewTimeDilation = 1.0f;
	if (bIsPaused)
	{
		NewTimeDilation = 0.0f;
		OnPause.Broadcast();
	}
	else
	{
		OnUnpause.Broadcast();
	}

	WARN_IF_NULL(GetWorld());
	if (GetWorld() != nullptr && GetWorld()->GetWorldSettings() != nullptr)
	{
		GetWorld()->GetWorldSettings()->SetTimeDilation(NewTimeDilation);
	}
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
			WARN_IF_NULL(CSP);
			if (CSP == nullptr)
			{
				continue;
			}

			if (CSP->IsCrisisActive())
			{
				continue;
			}

			CSP->SpawnCrisis(ThugEnemyClass);
			spawnedCrisis = true;
			break;
		}
	}

	//If all spawn points are active, see if there's any resolved ones we can force to clean up early
	if (!spawnedCrisis && AllowForcedCleanup)
	{
		for (const auto& CSP : CrisisSpawnPoints)
		{
			WARN_IF_NULL(CSP);
			if (CSP == nullptr)
			{
				continue;
			}

			if (CSP->IsCrisisActive() && CSP->IsCrisisResolved() && !CSP->IsPlayerInRange())
			{
				CSP->ForceCleanupNow();
				CSP->SpawnCrisis(ThugEnemyClass);
				spawnedCrisis = true;
				break;
			}
		}
	}

#if !UE_BUILD_SHIPPING
	if (DebugSpawnAllCrises)
	{
		for (const auto& CSP : CrisisSpawnPoints)
		{
			WARN_IF_NULL(CSP);
			if (CSP != nullptr && !CSP->IsCrisisActive())
			{
				CSP->SpawnCrisis(ThugEnemyClass);
			}
		}
	}
#endif //UE_BUILD_SHIPPING

	//If all else fails, we'll just skip this cycle
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
		WARN_IF_NULL(CSP);
		if (CSP == nullptr)
		{
			continue;
		}

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
		WARN_IF_NULL(CSP);
		if (CSP == nullptr)
		{
			continue;
		}

		double FearAmount = CSP->GetSecondsSinceCrisisStarted() / 5.0f; //Every 5 seconds a crisis is active, it increases our fear total by one
		WARN_IF(FearAmount < 0.0f);
		FearTotal += FMath::Clamp(FearAmount, 0.0, std::numeric_limits<double>::infinity());
	}

	if (DebugFastFearMeter)
	{
		FearTotal *= 10.0f;
	}

	if (DebugDisableFearMeter)
	{
		FearTotal = 0.0f;
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
	fNumCrisesResolved++;
	OnUpdateCrisisCount.Broadcast(GetNumActiveCrises());
	OnUpdateFearMeter.Broadcast(GetFearPercentage());

	//Clamp the time value so the next crisis doesn't spawn immediately after solving one
	if (GetNumActiveCrises() * 5.0f < fCurrentSpawnTime && fTimer > (GetNumActiveCrises() * 5.0f) - 5.0f)
	{
		fTimer = FMath::Clamp(fTimer, 0.0f, (GetNumActiveCrises() * 5.0f) - 5.0f);
	}
	else
	{
		fTimer = FMath::Clamp(fTimer, 0.0f, fCurrentSpawnTime - 1.0f);
	}
}

void AArcadeGameMode::GameOver()
{
	WARN_IF_NULL(GetWorld());

	bGameOver = true;

	if (GetWorld() != nullptr && GetWorld()->GetWorldSettings() != nullptr)
	{
		GetWorld()->GetWorldSettings()->SetTimeDilation(0.0f);
	}
	
	OnUpdateSessionTimer.Broadcast(fPlayStateTimer);
	OnGameOver.Broadcast();
}

void AArcadeGameMode::SpawnPlayerPawn(TSubclassOf<APawn> PawnToSpawn)
{
	if (aPlayStatePawn != nullptr)
	{
		aPlayStatePawn->Destroy();
		aPlayStatePawn = nullptr;
	}

	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	if (aPlayerStart != nullptr)
	{
		Location = aPlayerStart->GetActorLocation() + aPlayerStart->GetPlayerSpawnOffset();
		Rotation = aPlayerStart->GetActorRotation() + aPlayerStart->GetPlayerSpawnRotationOffset();
	}

	aPlayStatePawn = Cast<APawn>(GetWorld()->SpawnActor(PawnToSpawn, &Location, &Rotation));
	WARN_IF_NULL(aPlayStatePawn);
}