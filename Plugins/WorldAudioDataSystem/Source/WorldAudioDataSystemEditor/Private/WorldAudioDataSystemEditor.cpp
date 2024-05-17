// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldAudioDataSystemEditor.h"
#include "PointCloudSliceAndDiceRuleSet.h"
#include "SpawnWorldAudioDataRule.h"
#include "AssetToolsModule.h"
#include "Templates/SharedPointer.h"
#include "AssetTypeActions_WADVehAudioPreset.h"

#define LOCTEXT_NAMESPACE "FWorldAudioDataSystemEditorModule"

void FWorldAudioDataSystemEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// Add Rule Factory to Rule Factories for later registration
	RuleFactories.Add(new FSpawnWorldAudioDataFactory(Style));

	// Register all of the factories with the slice and dice system
	for (FSliceAndDiceRuleFactory* RuleFactory : RuleFactories)
	{
		UPointCloudSliceAndDiceRuleSet::RegisterRuleFactory(RuleFactory);
	}

	// Register the audio editor asset type actions.
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_WADVehAudioPreset));
}

void FWorldAudioDataSystemEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	for (FSliceAndDiceRuleFactory* RuleFactory : RuleFactories)
	{
		UPointCloudSliceAndDiceRuleSet::DeleteFactory(RuleFactory);
	}

	RuleFactories.Empty();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FWorldAudioDataSystemEditorModule, WorldAudioDataSystemEditor)