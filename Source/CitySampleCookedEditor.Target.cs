// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildBase;
using UnrealBuildTool;
using System.Collections.Generic;

[SupportedConfigurations(UnrealTargetConfiguration.Debug, UnrealTargetConfiguration.Development, UnrealTargetConfiguration.Test, UnrealTargetConfiguration.Shipping)]
public class CitySampleCookedEditorTarget : CitySampleEditorTarget
{
	public CitySampleCookedEditorTarget(TargetInfo Target) : base(Target)
	{
		if (!Unreal.IsEngineInstalled())
		{
			SetDefaultsForCookedEditor(bIsCookedCooker: false, bIsForExternalUse: true);
		}
	}
}
