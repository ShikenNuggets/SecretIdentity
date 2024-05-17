// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "SoundscapeSubsystem.h"
#include "WorldAudioDataSubsystem.h"
#include "WorldAudioMassProcessor.generated.h"

/**
 * Mass Processor that fills a SoundScape color point hash map from mass agent locations
 */
UCLASS()
class WORLDAUDIODATASYSTEM_API UWorldAudioMassProcessor : public UMassProcessor
{
	GENERATED_BODY()

	UWorldAudioMassProcessor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

protected:
	
	UPROPERTY(EditAnywhere, Category = "Mass|SoundScape", config)
	FName ColorPointCollectionName = TEXT("MassWorldAudio");

	UPROPERTY(EditAnywhere, Category = "Mass|SoundScape", config)
	FGameplayTag MovingVehicleColorPoint;
	
	UPROPERTY(EditAnywhere, Category = "Mass|SoundScape", config)
	FGameplayTag StoppedVehicleColorPoint;
	
	UPROPERTY(EditAnywhere, Category = "Mass|SoundScape", config)
	FGameplayTag MovingPedestrianColorPoint;
	
	UPROPERTY(EditAnywhere, Category = "Mass|SoundScape", config)
	FGameplayTag StoppedPedestrianColorPoint;

	// Hash Cell Update Timing
	UPROPERTY(EditAnywhere, Category = "Mass|SoundScape", config)
	float ColorPointHashUpdateTimeSeconds = 1.0f;
	
	// Speed above which traffic vehicles should use MovingVehicleColorPoint. StoppedVehicleColorPoint used otherwise.
	UPROPERTY(EditAnywhere, Category = "Mass|SoundScape", config)
	float VehicleMovingSpeedThreshold = 1.0f;
	
	// Speed above which traffic vehicles should use MovingVehicleColorPoint. StoppedVehicleColorPoint used otherwise.
	UPROPERTY(EditAnywhere, Category = "Mass|SoundScape", config)
	float PedestrianMovingSpeedThreshold = 1.0f;
	
	// The maximum number of individually audible nearby vehicles  
	UPROPERTY(EditAnywhere, Category = "Mass|SoundScape", config)
	int32 MaxIndividuallyAudibleVehicles = 20;
	

	FMassEntityQuery NearbyTrafficVehicleEntityQuery;
	
	FMassEntityQuery CrowdAgentEntityQuery;

	// temporary query to mark this processor as world accessing in terms of requirements and their thread-safety
	FMassEntityQuery DummyWorldAccessingQuery;

	UPROPERTY(Transient)
	USoundscapeColorPointHashMapCollection* ColorPointHashMapCollection;

	float ColorPointHashUpdateTimeSecondsRemaining = 0.0f;

	TArray<FVector> MovingVehicleColorPointLocations;
	TArray<FVector> StoppedVehicleColorPointLocations;
	TArray<FVector> MovingPedestrianColorPointLocations;
	TArray<FVector> StoppedPedestrianColorPointLocations;
	
	TArray<FWorldAudioDataVehicleInfo> IndividuallyAudibleVehicles;
};


template<>
struct TMassExternalSubsystemTraits<USoundscapeSubsystem>
{
	enum
	{
		GameThreadOnly = true,
		ThreadSafeRead = true,
		ThreadSafeWrite = false,
	};
};
