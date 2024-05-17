// Copyright Epic Games, Inc. All Rights Reserved.

#include "HoverDroneVolumeManager.h"
#include "HoverDroneSpeedLimitBox.h"
#include "Engine/BlockingVolume.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "EngineUtils.h"

static const FName HoverDroneVolumeTag("Drone");

void UHoverDroneVolumeManager::Initialize(FSubsystemCollectionBase& Collection)
{
	OnLevelRemovedFromWorldHandle = FWorldDelegates::LevelRemovedFromWorld.AddUObject(this, &ThisClass::OnLevelRemovedFromWorld);
	OnLevelAddedToWorldHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(this, &ThisClass::OnLevelAddedToWorld);
	PostGarbageCollectHandle = FCoreUObjectDelegates::GetPostGarbageCollect().AddUObject(this, &ThisClass::PostGarbageCollect);

	UWorld* World = GetGameInstance()->GetWorld();

	for (TActorIterator<ABlockingVolume> It(World); It; ++It)
	{
		ABlockingVolume* Actor = *It;
		if (IsValid(Actor) && Actor->ActorHasTag(HoverDroneVolumeTag))
		{
			BlockingVolumes.Add(Actor);
		}
	}

	for (TActorIterator<AHoverDroneSpeedLimitBox> It(World); It; ++It)
	{
		AHoverDroneSpeedLimitBox* Actor = *It;
		if (IsValid(Actor))
		{
			SpeedLimitBoxes.Add(Actor);
		}
	}
}

void UHoverDroneVolumeManager::Deinitialize()
{
	SpeedLimitBoxes.Empty();
	BlockingVolumes.Empty();

	FWorldDelegates::LevelRemovedFromWorld.Remove(OnLevelRemovedFromWorldHandle);
	FWorldDelegates::LevelAddedToWorld.Remove(OnLevelAddedToWorldHandle);
	FCoreUObjectDelegates::GetPostGarbageCollect().Remove(PostGarbageCollectHandle);

	OnLevelRemovedFromWorldHandle = FDelegateHandle();
	OnLevelAddedToWorldHandle = FDelegateHandle();
	PostGarbageCollectHandle = FDelegateHandle();
}

void UHoverDroneVolumeManager::PostGarbageCollect()
{
	for (auto It = SpeedLimitBoxes.CreateIterator(); It; ++It)
	{
		if (!IsValid(*It))
		{
			It.RemoveCurrent();
		}
	}

	for (auto It = BlockingVolumes.CreateIterator(); It; ++It)
	{
		if (!IsValid(*It))
		{
			It.RemoveCurrent();
		}
	}
}

void UHoverDroneVolumeManager::OnLevelRemovedFromWorld(class ULevel* Level, class UWorld* World)
{
	if (GetGameInstance()->GetWorld() != World)
	{
		return;
	}
	
	for (auto It = SpeedLimitBoxes.CreateIterator(); It; ++It)
	{
		if (*It == nullptr || (*It)->IsPendingKillPending() || (*It)->IsInLevel(Level))
		{
			It.RemoveCurrent();
		}
	}

	for (auto It = BlockingVolumes.CreateIterator(); It; ++It)
	{
		if (*It == nullptr || (*It)->IsPendingKillPending() || (*It)->IsInLevel(Level))
		{
			It.RemoveCurrent();
		}
	}
}

void UHoverDroneVolumeManager::OnLevelAddedToWorld(class ULevel* Level, class UWorld* World)
{
	if (GetGameInstance()->GetWorld() != World)
	{
		return;
	}

	// This feels pretty expensive.
	for (AActor* Actor : Level->Actors)
	{
		if (AHoverDroneSpeedLimitBox* SpeedLimitBox = Cast<AHoverDroneSpeedLimitBox>(Actor))
		{
			SpeedLimitBoxes.Add(SpeedLimitBox);
		}
		else if (ABlockingVolume* BlockingVolume = Cast<ABlockingVolume>(Actor))
		{
			if (BlockingVolume->ActorHasTag(HoverDroneVolumeTag))
			{
				BlockingVolumes.Add(BlockingVolume);
			}
		}
	}
}



