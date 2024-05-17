// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldAudioDataGameplayScript.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class WORLDAUDIODATASYSTEM_API AWorldAudioDataGameplayScript : public AActor
{
	GENERATED_BODY()

public:

	// Sets default values for this actor's properties
	AWorldAudioDataGameplayScript();

#if WITH_EDITOR
	virtual bool CanChangeIsSpatiallyLoadedFlag() const override { return false; }
#endif

};
