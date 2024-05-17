// Copyright Epic Games, Inc. All Rights Reserved.

#include "CrossfaderEditor.h"
#include "AssetToolsModule.h"
#include "Templates/SharedPointer.h"
#include "AssetTypeActions_MixStateBank.h"

IMPLEMENT_MODULE(FCrossfaderEditorModule, CrossfaderEditor)

#define LOCTEXT_NAMESPACE "FCrossfaderEditorModule"

void FCrossfaderEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

		// Register the audio editor asset type actions.
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_MixStateBank));
}

void FCrossfaderEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
