// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "SecretIdentity/Actors/CrisisSpawnPoint.h"

#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

#include "SecretIdentity/UE_Helpers.h"

ACrisisSpawnPoint::ACrisisSpawnPoint()
{
}

void ACrisisSpawnPoint::BeginPlay()
{
	bIsCrisisActive = false;
	WARN_IF(TypeToSpawn >= CrisisType::Count);
}

bool ACrisisSpawnPoint::IsCrisisActive() const
{
	return bIsCrisisActive;
}

void ACrisisSpawnPoint::SpawnCrisis(TSubclassOf<ACharacter> ThugCharacterBP)
{
	WARN_IF_NULL(ThugCharacterBP);

	if (bIsCrisisActive)
	{
		return; //We shouldn't have two crises spawned in the same location at the same time
	}

	bIsCrisisActive = true;

	FVector CurrentLocation = GetActorLocation();
	FRotator CurrentRotation = GetActorRotation();

	AActor* ThugCharacter = GetWorld()->SpawnActor(ThugCharacterBP, &CurrentLocation, &CurrentRotation);
	if (ThugCharacter != nullptr)
	{
		LOG_MSG(TEXT("Spawned crisis of type ") + FString::FromInt(static_cast<int32>(TypeToSpawn)));
		ThugCharacter->OnEndPlay.AddDynamic(this, &ACrisisSpawnPoint::OnCrisisActorEndPlay);
	}
	else
	{
		LOG_MSG_WARNING("Could not spawn Thug Actor. Check for warnings/errors in the Output log");
	}
}

void ACrisisSpawnPoint::OnCrisisActorEndPlay(AActor* ActorDestroyed, EEndPlayReason::Type Reason)
{
	LOG_MSG("Crisis is over - spawn point reset");
	bIsCrisisActive = false;
}