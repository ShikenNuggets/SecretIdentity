// Copyright Carter Rennick, 2024. All Rights Reserved.

#include "SecretIdentity/Actors/CrisisSpawnPoint.h"

#include "SecretIdentity/UE_Helpers.h"

ACrisisSpawnPoint::ACrisisSpawnPoint(){}

void ACrisisSpawnPoint::BeginPlay(){
	WARN_IF(TypeToSpawn >= CrisisType::Count);
}