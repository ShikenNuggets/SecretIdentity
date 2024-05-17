// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WorldAudioDataGameplayScript.h"
#include "Tickable.h"
#include "MassExternalSubsystemTraits.h"
#include "WorldAudioDataSubsystem.generated.h"

class UAudioComponent;
class UWorldAudioDataVehAudioController;
class UWADVehAudioPreset;
class UWorldContext;
class UWorldAudioReverbDataCollection;
class UWorldAudioDataSettings;

UCLASS()
class WORLDAUDIODATASYSTEM_API UContinuousSoundSystemVectorCollection : public UObject
{
	GENERATED_BODY()

public:

	// Pointcloud Data Key
	UPROPERTY(VisibleAnywhere)
	FString PointcloudDataKey = TEXT("None");

	// Vector Array
	UPROPERTY(VisibleAnywhere)
	TArray<FVector> VectorCollection;
};

UCLASS()
class WORLDAUDIODATASYSTEM_API UContinuousSound : public UObject
{
	GENERATED_BODY()

public:

	void InitializeContinuousSound(const UObject* WorldContext, FString InDataKey, USoundBase* Sound);

	void UpdateAudioComponent(const TArray<FVector>& DataLocations, const FVector ListenerLocation);

	const FString& GetDataKey() const;

	bool NeedsNewAudioComponent() const;

private:
	// Pointcloud data key
	UPROPERTY(Transient)
	FString DataKey = TEXT("None");

	// Playing audio component
	UPROPERTY(Transient)
	UAudioComponent* AudioComponent = nullptr;

	FVector ComponentLocation = FVector::ZeroVector;
};

struct FWorldAudioDataVehicleInfo
{
	// Unique entity ID, persistent for the life of the vehicle
	// @see FMassEntityHandle::Index
	int32 Id;

	// Squared distance to closest viewer
	// @see FMassRepresentationLODFragment::ClosestViewerDistanceSq 
	float ClosestViewerDistanceSq;

	// Value scaling from 0 to 3, 0 highest LOD we support and 3 being completely off LOD
	// @see FMassRepresentationLODFragment::LODSignificance 
	float LODSignificance;

	// The audio controller to use for this vehicle
	// @see FMassTrafficVehicleTypeData::AudioController
	FName AudioController;

	// The world space location of this vehicle
	FVector Location;

	// World space linear velocity of this vehicle in cm/s 
	FVector LinearVelocity;
	
	// EMassTrafficVehicleDamageState
	// 0 = None, 1 = Damaged, 2 = Totaled (undreiveable), 3 = Repairing (playing the derez effect), 4 = Irreparable (derez has finished, car is waiting to be teleported to new location)
	uint8 VehicleDamageState;  
};

/**
 * 
 */
UCLASS()
class WORLDAUDIODATASYSTEM_API UWorldAudioDataSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	
	// Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	// End USubsystem

	void UpdateWorldAudioDataVehAudioControllers(const TArray<FWorldAudioDataVehicleInfo>& WorldAudioDataVehicleInfo);

	UFUNCTION(BlueprintCallable)
	void ActivateVehAudioControllers();

	UFUNCTION(BlueprintCallable)
	void DeactivateVehAudioControllers();

private:
	bool bVehAudioControllersActive = false;

	UPROPERTY()
	const UWorldAudioDataSettings* WorldAudioDataSettings;

public:
	// Begin FTickableGameObject
	void Tick(float DeltaTime) override;
	bool IsTickable() const override;
	bool IsTickableInEditor() const override;
	bool IsTickableWhenPaused() const override;
	TStatId GetStatId() const override;
	// End FTickableGameObject

	void AddContinuousSoundSystemVectorCollection(const TArray<UContinuousSoundSystemVectorCollection*>& ContinuousSoundSystemVectorCollectionsIn);

	UFUNCTION(BlueprintCallable)
	void ActivateContinuousSoundSystem();

	UFUNCTION(BlueprintCallable)
	void DeactivateContinuousSoundSystem();

	UFUNCTION(BlueprintCallable)
	bool GetWorldAudioDataGameplayScript(AWorldAudioDataGameplayScript*& WorldAudioDataGameplayScriptOut);

private:

	bool bContinuousSoundSystemActive = false;

	UPROPERTY(Transient)
	AWorldAudioDataGameplayScript* WorldAudioDataGameplayScript = nullptr;

	bool bIsTickable = false;

	UPROPERTY()
	TMap <int32, UWorldAudioDataVehAudioController*> VehAudioControllers;

	UPROPERTY()
	TMap<FName, UWADVehAudioPreset*> VehAudioControllerPresetMap;

	FVector CurrentPawnLocation = FVector::ZeroVector;
	FVector ListenerLocation = FVector::ZeroVector;
	FVector ListenerForward = FVector::ZeroVector;
	FVector ListenerUp = FVector::ZeroVector;

	// Cached continuous sound data collection
	UPROPERTY(Transient)
	TArray<UContinuousSoundSystemVectorCollection*> ContinuousSoundSystemVectorCollections;

	// Active continuous sound players
	UPROPERTY(Transient)
	TArray< UContinuousSound*> ActiveContinuousSounds;

	// String tags for continuous sounds that follow the pawn location
	TSet<FString> ContinuousPawnSoundTags;

	// Cached continuous sound data collection
	UPROPERTY(Transient)
	TArray<UContinuousSoundSystemVectorCollection*> ContinuousPawnSoundCollections;

public:

	void AddReverbDataCollection(UWorldAudioReverbDataCollection* ReverbDataCollection);

private:

	// Cached reverb data collection
	UPROPERTY(Transient)
	TArray< UWorldAudioReverbDataCollection*> WorldAudioReverbDataCollections;

	uint8 DetermineReverbValueAtLocation(FVector InLocation);

	bool bReverbEffectActive = false;
};

template<>
struct TMassExternalSubsystemTraits<UWorldAudioDataSubsystem>
{
	enum
	{
		GameThreadOnly = true,
		ThreadSafeRead = true,
		ThreadSafeWrite = false,
	};
};
