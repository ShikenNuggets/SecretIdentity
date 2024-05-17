// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"
#include "Styling/SlateStyle.h"

class FSliceAndDiceRuleFactory;

class FWorldAudioDataSystemEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Holds the plug-ins style set. */
	TSharedPtr<ISlateStyle> Style;

	TArray<FSliceAndDiceRuleFactory*> RuleFactories;
};
