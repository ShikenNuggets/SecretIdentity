// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CitySampleTarget : TargetRules
{
	public CitySampleTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("CitySample");

		if (BuildEnvironment == TargetBuildEnvironment.Unique)
		{
			bUseLoggingInShipping = true;
		}
	}
}
