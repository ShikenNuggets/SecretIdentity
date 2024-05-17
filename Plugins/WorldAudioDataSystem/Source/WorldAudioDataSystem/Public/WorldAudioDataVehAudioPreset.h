// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "WorldAudioDataVehAudioPreset.generated.h"

class UMetaSoundSource;
class USoundBase;

/**
 * 
 */
UCLASS()
class WORLDAUDIODATASYSTEM_API UWADVehAudioPreset : public UObject
{
	GENERATED_BODY()

public:
	// MetaSound for Engine Simulation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "EngineSimulation"))
	UMetaSoundSource* EngineMetaSound;

	// MetaSound Float Input Name for passing Velocity to the MetaSound
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "EngineSimulation"))
	FName VelocityInputName = TEXT("Velocity");

	// MetaSound Integer Input Name for passing Velocity to the MetaSound
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "EngineSimulation"))
	FName RandomSeedInputName = TEXT("Seed");

	// MetaSound Integer Input Name for passing Velocity to the MetaSound
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "EngineSimulation"))
	FName MinEnginePitchOffsetName = TEXT("MinEnginePitchOffset");

	// MetaSound Integer Input Name for passing Velocity to the MetaSound
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "EngineSimulation"))
	float MinEnginePitchOffset = 0.0f;

	// MetaSound Integer Input Name for passing Velocity to the MetaSound
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "EngineSimulation"))
	FName MaxEnginePitchOffsetName = TEXT("MaxEnginePitchOffset");

	// MetaSound Integer Input Name for passing Velocity to the MetaSound
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "EngineSimulation"))
	float MaxEnginePitchOffset = 0.0f;

	// Sound for braking
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "CarSound"))
	USoundBase* BrakingSound;

	// Sound for honking horn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "CarSound"))
	USoundBase* HonkingSound;

	// Horn Honk Distance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "CarSound"))
	float HornHonkDistance = 500.0f;

	// Speed of sound in Meters Per Second
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "InterpretiveData"))
	float SpeedOfSoundMPS = 344.0f;
};
