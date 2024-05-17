// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "WorldAudioDataVehAudioController.generated.h"

class UWADVehAudioPreset;
class UAudioComponent;

UCLASS( ClassGroup=(WorldAudioData), meta=(BlueprintSpawnableComponent) )
class WORLDAUDIODATASYSTEM_API UWorldAudioDataVehAudioController : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWorldAudioDataVehAudioController();

	UFUNCTION(BlueprintCallable)
	void SetPreset(UWADVehAudioPreset* VehicleAudioPreset, int32 Seed);

	UFUNCTION(BlueprintCallable)
	void SetVelocity(float InSpeed);

	UFUNCTION(BlueprintCallable)
	void StopController();

	// Cached preset
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UWADVehAudioPreset* WADVehicleAudioPreset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RandomSeed = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinEnginePitchOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxEnginePitchOffset = 0.0f;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Called when the game ends
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;;

public:	
	void ManualUpdate(const float DeltaTime, const FVector PlayerPawnLocation);

private:

	void UpdateCalculations(float DeltaTime, const FVector PawnLocation);
	void UpdateEngineMetaSound();
	void UpdateAudioComponentPitchMods();
	void UpdateAudioComponentLocations();
	void HonkHorn();

	// Main engine AC
	UPROPERTY()
	UAudioComponent* MainEngineAudioComponent;

	// Main engine AC
	UPROPERTY()
	UAudioComponent* HornHonkAudioComponent;

	// Cached world location
	FVector PreviousLocation = FVector::ZeroVector;

	// Calculated values
	float CurrentSpeed = 0.0f;
	float PreviousDistanceFromPawn = 0.0f;
	float DeltaDistanceFromPawn = 0.0f;
	float SpeedOfSound = 34400.0f;
	float CurrentPitchMod = 1.0f;

	const float MinRefreshPeriod = 0.0083f;
	const float MaxRefreshPeriod = 0.0666f;
};
