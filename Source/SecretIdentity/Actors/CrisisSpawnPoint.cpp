// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "SecretIdentity/Actors/CrisisSpawnPoint.h"

#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

#include "SecretIdentity/UE_Helpers.h"
#include "SecretIdentity/Characters/EnemyCharacter.h"

ACrisisSpawnPoint::ACrisisSpawnPoint()
{
}

void ACrisisSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	bIsCrisisActive = false;
	WARN_IF(TypeToSpawn >= CrisisType::Count);
}

bool ACrisisSpawnPoint::IsCrisisActive() const
{
	return bIsCrisisActive;
}

bool ACrisisSpawnPoint::IsCrisisActiveAndNotResolved() const
{
	return bIsCrisisActive && !bIsActiveCrisisResolved;
}

void ACrisisSpawnPoint::SpawnCrisis(TSubclassOf<ACharacter> ThugCharacterBP)
{
	WARN_IF_NULL(ThugCharacterBP);

	if (bIsCrisisActive)
	{
		return; //Do not spawn two crises in the same location at the same time
	}

	ActivateCrisis();

	FVector CurrentLocation = GetActorLocation();
	FRotator CurrentRotation = GetActorRotation();

	AEnemyCharacter* ThugCharacter = Cast<AEnemyCharacter>(GetWorld()->SpawnActor(ThugCharacterBP, &CurrentLocation, &CurrentRotation));
	if (ThugCharacter != nullptr)
	{
		ThugCharacter->OnDeathDelegate.AddUObject(this, &ACrisisSpawnPoint::OnCrisisActorDead);
		ThugCharacter->OnEndPlay.AddDynamic(this, &ACrisisSpawnPoint::OnCrisisActorEndPlay);
		tCrisisActors.Add(ThugCharacter);
	}
	else
	{
		LOG_MSG_WARNING("Could not spawn Thug Actor. Check for warnings/errors in the Output log");
	}
}

void ACrisisSpawnPoint::OnCrisisActorEndPlay(AActor* ActorDestroyed, EEndPlayReason::Type Reason)
{
	bIsCrisisActive = false;
}

double ACrisisSpawnPoint::GetSecondsSinceCrisisStarted() const
{
	if (!bIsCrisisActive || bIsActiveCrisisResolved)
	{
		return 0.0f;
	}

	FDateTime Now = FDateTime::UtcNow();
	return UE_Helpers::GetDifferenceInSeconds(fActiveCrisisStartTime, Now);
}

FVector ACrisisSpawnPoint::GetAverageCrisisActorLocation() const
{
	return UGameplayStatics::GetActorArrayAverageLocation(tCrisisActors);
}

TArray<FVector> ACrisisSpawnPoint::GetAllCrisisActorPositions() const
{
	TArray<FVector> Positions;

	for(const auto& A : tCrisisActors)
	{
		WARN_IF_NULL(A);
		if (A != nullptr)
		{
			Positions.Add(A->GetActorLocation());
		}
	}

	return Positions;
}

void ACrisisSpawnPoint::ActivateCrisis()
{
	bIsCrisisActive = true;
	bIsActiveCrisisResolved = false;
	fActiveCrisisStartTime = FDateTime::UtcNow();
}

void ACrisisSpawnPoint::ResolveCrisis()
{
	tCrisisActors.Empty();

	bIsActiveCrisisResolved = true;
	fActiveCrisisStartTime = FDateTime();
}

void ACrisisSpawnPoint::OnCrisisActorDead(AEnemyCharacter* Enemy)
{
	ResolveCrisis(); //Very important that this is called BEFORE the broadcast
	OnCrisisResolved.Broadcast(this);
}