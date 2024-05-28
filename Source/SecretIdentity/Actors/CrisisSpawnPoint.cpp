// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "SecretIdentity/Actors/CrisisSpawnPoint.h"

#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

#include "SecretIdentity/UE_Helpers.h"
#include "SecretIdentity/Characters/EnemyCharacter.h"

ACrisisSpawnPoint::ACrisisSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ACrisisSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	bIsCrisisActive = false;
	WARN_IF(TypeToSpawn >= CrisisType::Count);
}

void ACrisisSpawnPoint::Tick(float DeltaTime)
{
	if (IsCrisisActive() && bIsActiveCrisisResolved && !bIsCleaningUp)
	{
		if (aPlayerPawn == nullptr)
		{
			bIsCleaningUp = true;
			DespawnAllCrisisActors();
		}
		else
		{
			double DistanceFromPlayer = FMath::Abs(FVector::Distance(GetActorLocation(), aPlayerPawn->GetActorLocation()));
			if (DistanceFromPlayer > DespawnDistanceWhenResolved)
			{
				bIsCleaningUp = true;
				DespawnAllCrisisActors();
			}
		}
	}
}

bool ACrisisSpawnPoint::IsCrisisActive() const
{
	return bIsCrisisActive;
}

bool ACrisisSpawnPoint::IsCrisisResolved() const
{
	return bIsActiveCrisisResolved;
}

bool ACrisisSpawnPoint::IsCrisisActiveAndNotResolved() const
{
	return bIsCrisisActive && !bIsActiveCrisisResolved;
}

void ACrisisSpawnPoint::SpawnCrisis(TSubclassOf<ACharacter> ThugCharacterBP)
{
	WARN_IF_NULL(ThugCharacterBP);
	WARN_IF_NULL(GetWorld());

	if (bIsCrisisActive)
	{
		return; //Do not spawn two crises in the same location at the same time
	}

	ActivateCrisis();

	FVector CurrentLocation = GetActorLocation();
	FRotator CurrentRotation = GetActorRotation();

	AEnemyCharacter* ThugCharacter = nullptr;
	if (GetWorld() != nullptr && ThugCharacterBP != nullptr)
	{
		ThugCharacter = Cast<AEnemyCharacter>(GetWorld()->SpawnActor(ThugCharacterBP, &CurrentLocation, &CurrentRotation));
	}
	else if (ThugCharacterBP != nullptr)
	{
		LOG_MSG_WARNING("ThugCharacterBP was null!");
	}

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

void ACrisisSpawnPoint::ForceCleanupNow()
{
	if (!IsCrisisActive() || !IsCrisisResolved())
	{
		return;
	}

	for (const auto& A : tCrisisActors)
	{
		WARN_IF_NULL(A);
		if (A != nullptr)
		{
			A->SetLifeSpan(0.001f);
		}
	}

	tCrisisActors.Empty();

	fCooldownTimerHandle.Invalidate();

	bIsCrisisActive = false;
	bIsActiveCrisisResolved = false;
	bIsCleaningUp = false;
}

void ACrisisSpawnPoint::OnCrisisActorEndPlay(AActor* ActorDestroyed, EEndPlayReason::Type Reason)
{
	WARN_IF_NULL(ActorDestroyed);
	WARN_IF_NULL(GetWorld());

	tCrisisActors.Remove(ActorDestroyed);
	if (tCrisisActors.IsEmpty() && GetWorld() != nullptr)
	{
		if (fCooldownTimerHandle.IsValid())
		{
			fCooldownTimerHandle.Invalidate();
		}

		GetWorld()->GetTimerManager().SetTimer(fCooldownTimerHandle, FTimerDelegate::CreateLambda([&]
		{
			bIsCrisisActive = false;
			bIsCleaningUp = false;
		}), CooldownTime, false);
	}
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
	bIsCleaningUp = false;
	fActiveCrisisStartTime = FDateTime::UtcNow();
}

void ACrisisSpawnPoint::ResolveCrisis()
{
	bIsActiveCrisisResolved = true;
	fActiveCrisisStartTime = FDateTime();
}

void ACrisisSpawnPoint::DespawnAllCrisisActors()
{
	for (const auto& A : tCrisisActors)
	{
		A->SetLifeSpan(1.0f);
	}
}

void ACrisisSpawnPoint::OnCrisisActorDead(AEnemyCharacter* Enemy)
{
	bool IsAnyEnemyAlive = false;
	for (const auto& A : tCrisisActors)
	{
		AEnemyCharacter* CurrentEnemy = Cast<AEnemyCharacter>(A);
		if (CurrentEnemy != nullptr && !CurrentEnemy->IsDead())
		{
			IsAnyEnemyAlive = true;
			break;
		}
	}

	if (!IsAnyEnemyAlive)
	{
		ResolveCrisis(); //Very important that this is called BEFORE the broadcast
		OnCrisisResolved.Broadcast(this);
	}
}

void ACrisisSpawnPoint::OnPlayStateBegins(APawn* NewPawn)
{
	aPlayerPawn = NewPawn;
}