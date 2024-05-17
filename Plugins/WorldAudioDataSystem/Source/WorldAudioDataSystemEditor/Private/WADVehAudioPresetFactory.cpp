// Copyright Epic Games, Inc. All Rights Reserved.


#include "WADVehAudioPresetFactory.h"
#include "WorldAudioDataVehAudioPreset.h"

UWADVehAudioPresetFactory::UWADVehAudioPresetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UWADVehAudioPreset::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UWADVehAudioPresetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UWADVehAudioPreset>(InParent, Name, Flags);

}