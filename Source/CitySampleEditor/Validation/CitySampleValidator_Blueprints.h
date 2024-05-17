// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CitySampleEditorValidator.h"
#include "CitySampleValidator_Blueprints.generated.h"

UCLASS()
class UCitySampleValidator_Blueprints : public UCitySampleEditorValidator
{
	GENERATED_BODY()

public:
	UCitySampleValidator_Blueprints();

	static void SetShouldLoadReferencingBlueprintsInEditor(bool bNewShouldLoadReferencingBlueprintsInEditor);

protected:
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context);

private:
	static bool bShouldLoadReferencingBlueprintsInEditor;
};