// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "MixStateBank.generated.h"

/** This Struct allows designers to associate MixStates with SoundControlBusMixes. */
USTRUCT()
struct CROSSFADER_API FCrossfaderMixPair
{
	GENERATED_BODY()

	/** The GameplayTag namespace used as a MixState, when set the Crossfader Subsystem will trigger the associated SoundControlBusMix */
	UPROPERTY(EditAnywhere, meta = (Categories = "Crossfader"))
	FGameplayTag MixState;

	/** ControlBusMix activated by the associated MixState */
	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "/Script/AudioModulation.SoundControlBusMix"))
	FSoftObjectPath ControlBusMix;

	FCrossfaderMixPair()
	{
	}
};

/** The MixStateBank allows designers to add or remove MixStates from the Subsystem master bank during runtime. */
UCLASS(BlueprintType)
class CROSSFADER_API UMixStateBank : public UObject
{
	GENERATED_BODY()

public:

	/** The MixState associations stored in this MixStateBank */
	UPROPERTY(EditAnywhere, Category = "MixStates")
	TArray<FCrossfaderMixPair> MixStates;
};

