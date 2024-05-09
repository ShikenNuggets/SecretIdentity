// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "SecretIdentity/Actors/CrisisSpawnPoint.h"

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

void ACrisisSpawnPoint::SpawnCrisis()
{
	if (bIsCrisisActive)
	{
		return; //We shouldn't have two crises spawned in the same location at the same time
	}

	bIsCrisisActive = true;

	//TODO - Actually spawn the enemy/etc
	LOG_MSG(TEXT("Spawning crisis of type ") + FString::FromInt(static_cast<int32>(TypeToSpawn)));
}