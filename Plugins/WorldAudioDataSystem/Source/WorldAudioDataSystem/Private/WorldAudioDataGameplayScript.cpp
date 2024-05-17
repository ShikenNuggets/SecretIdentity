// Copyright Epic Games, Inc. All Rights Reserved.

#include "WorldAudioDataGameplayScript.h"

#if WITH_EDITORONLY_DATA
#include "Modules/ModuleManager.h"
#include "MetasoundEditorModule.h"
#endif // WITH_EDITORONLY_DATA

#define LOCTEXT_NAMESPACE "FWorldAudioDataSystemModule"


AWorldAudioDataGameplayScript::AWorldAudioDataGameplayScript()
{
	// The MetaSoundEditor module must be loaded here to ensure
	// referenced graph editor types are loaded prior to script being loaded.
	// Otherwise, MetaSoundEditorGraph data will fail to load properly in the editor.
#if WITH_EDITORONLY_DATA
	FModuleManager::LoadModuleChecked<Metasound::Editor::IMetasoundEditorModule>("MetaSoundEditor");
	bIsSpatiallyLoaded = false;
#endif // WITH_EDITORONLY_DATA

	SetActorHiddenInGame(true);
}