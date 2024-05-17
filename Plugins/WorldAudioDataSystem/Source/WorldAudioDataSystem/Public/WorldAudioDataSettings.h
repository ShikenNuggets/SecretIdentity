// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPath.h"
#include "WorldAudioDataGameplayScript.h"
#include "WorldAudioDataSettings.generated.h"

USTRUCT()
struct WORLDAUDIODATASYSTEM_API FMantleDataIntensityToEffectPresetChain
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	uint8 MinimumEffectRange = 0;

	// Soundscape Palette Collection
	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "/Script/Engine.SoundEffectSubmixPreset"))
	TArray<FSoftObjectPath> SubmixEffectPresetChain;
};

USTRUCT()
struct WORLDAUDIODATASYSTEM_API FMantleDataSubmixToEffectMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "/Script/Engine.SoundSubmix"))
	FSoftObjectPath SoundSubmix;

	UPROPERTY(EditAnywhere)
	TArray<FMantleDataIntensityToEffectPresetChain> IntensityToEffectMap;
};

/**
 * 
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "World Audio Data"))
class WORLDAUDIODATASYSTEM_API UWorldAudioDataSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
// 	// MetaData Key Value to query point cloud data
// 	UPROPERTY(config, EditAnywhere, Category = "SoundscapeColorPointMapping")
// 	FString MantleColorPointMetaDataKey = TEXT("type");

	// Mantle Data Value to Soundscape ColorPoint Mapping
	UPROPERTY(config, EditAnywhere, Category = "SoundscapeColorPointMapping")
	TMap<FString, FGameplayTag> MantleTypeToSoundscapeColorPointMap;

	// Tags to assign continuous sounds to the pawn location
	UPROPERTY(config, EditAnywhere, Category = "WorldAudioDataContinuousSound")
	TSet<FString> ContinuousPawnTags;

	// Continuous Sound for World Audio Data
	UPROPERTY(config, EditAnywhere, Category = "WorldAudioDataContinuousSound", meta = (AllowedClasses = "/Script/Engine.SoundBase"))
	TMap<FString, FSoftObjectPath> ContinuousSoundMap;

	// MetaData Key Value to query point cloud data
	UPROPERTY(config, EditAnywhere, Category = "EffectsMapping")
	FString MantleEffectsMetaDataKey = TEXT("reverb");

	UPROPERTY(config, EditAnywhere, Category = "EffectsMapping")
	float MaxEffectAltitude = 1000.0f;

	UPROPERTY(config, EditAnywhere, Category = "EffectsMapping")
	TArray<FMantleDataSubmixToEffectMapping> EffectMap;

	UPROPERTY(config, EditAnywhere, Category = "WorldAudioDataGameplay")
	TSubclassOf<AWorldAudioDataGameplayScript> WorldAudioDataGameplayScript;

	UPROPERTY(config, EditAnywhere, Category = "WorldAudioDataGameplay", meta = (AllowedClasses = "/Script/WorldAudioDataSystem.WADVehAudioPreset"))
	TMap<FName, FSoftObjectPath> MASSTrafficCarConfigurationPresetMap;

};
