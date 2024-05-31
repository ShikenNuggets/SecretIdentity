// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SecretIdentityTarget : TargetRules
{
	public SecretIdentityTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("SecretIdentity");
		ExtraModuleNames.Add("CitySample");

		if (BuildEnvironment == TargetBuildEnvironment.Unique)
		{
			bUseLoggingInShipping = true;
		}
	}
}
