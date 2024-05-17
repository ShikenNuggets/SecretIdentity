// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "WorldAudioDataClusterActor.generated.h"

class USoundscapeColorPointHashMapCollection;
class UContinuousSoundSystemVectorCollection;

/*
 * Uses UPROPERTY for serialization purposes
 */
USTRUCT()
struct WORLDAUDIODATASYSTEM_API FWorldAudioReverbData
{
	GENERATED_BODY()

	// Average value of the cell
	UPROPERTY(VisibleAnywhere)
	uint8 DataCellAverage = 0;

	// Number of data points in cell
	UPROPERTY(VisibleAnywhere)
	uint8 DataCellWeighting = 0;
};

UCLASS()
class WORLDAUDIODATASYSTEM_API UWorldAudioReverbDataCollection : public UObject
{
	GENERATED_BODY()

public:

	void AddReverbDataPoint(const FVector DataLocation, const float DataValue);

	uint8 EvaluateDataAtLocation(const FVector Location);
	uint8 EvaluateDataAtLocation(const FVector Location, uint8& OutWeighting);

	void ClearDataCollection();

private:
	UPROPERTY()
	TMap<uint32, FWorldAudioReverbData> DataCollection;

	uint32 Generate2DLocationHash(const FVector Location);

	void UpdateCellData(FWorldAudioReverbData* DataToUpdate, const float IncomingValue);
};

// Type only needed in Editor
USTRUCT()
struct WORLDAUDIODATASYSTEM_API FWorldAudioDataClusterActorDataSummary
{
	GENERATED_BODY()

	// Hash Cell Width for LOD1
	UPROPERTY(VisibleAnywhere)
	float LOD1ColorPointHashWidth = 500.0f;

	// Hash Cell LOD1 Max Distance
	UPROPERTY(VisibleAnywhere)
	float LOD1ColorPointHashDistance = 5000.0f;

	UPROPERTY(VisibleAnywhere)
	TMap<FGameplayTag, int32> LOD1DensityTotals;

	// Hash Cell Width for LOD2
	UPROPERTY(VisibleAnywhere)
	float LOD2ColorPointHashWidth = 2500.0f;

	// Hash Cell LOD2 Max Distance
	UPROPERTY(VisibleAnywhere)
	float LOD2ColorPointHashDistance = 10000.0f;

	UPROPERTY(VisibleAnywhere)
	TMap<FGameplayTag, int32> LOD2DensityTotals;

	// Hash Cell Width for LOD3
	UPROPERTY(VisibleAnywhere)
	float LOD3ColorPointHashWidth = 10000.0f;

	UPROPERTY(VisibleAnywhere)
	TMap<FGameplayTag, int32> LOD3DensityTotals;
};

UCLASS()
class WORLDAUDIODATASYSTEM_API AWorldAudioDataClusterActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWorldAudioDataClusterActor();

	// Cached Hash Map data 
	UPROPERTY()
	USoundscapeColorPointHashMapCollection* SoundscapeColorPointHashMapCollection;

	// Cached continuous sound data collection
	UPROPERTY()
	TArray<UContinuousSoundSystemVectorCollection*> ContinuousSoundSystemVectorCollections;

	void InitializeActor();

#if WITH_EDITOR
	// Generates data for the Data Summary, only needed in Editor
	void GenerateSummary();
#endif

#if WITH_EDITORONLY_DATA
	// Data only needed in Editor for debug
	UPROPERTY(VisibleAnywhere)
	FWorldAudioDataClusterActorDataSummary ClusterActorDataSummary;
#endif

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Overridable function called whenever this actor is being removed from a level */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

	// Actor tick
	virtual void Tick(float DeltaSeconds) override;

private:
	// Flag for attempting to submit data on tick
	bool bRetryToSendSoundscapeData = false;

	// Flag for attempting to submit data on tick
	bool bRetryToSendWorldAudioData = false;

public:
	UPROPERTY(VisibleAnywhere)
	UWorldAudioReverbDataCollection* ReverbCollection;
};
