// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CitySampleEditorTarget : TargetRules
{
	public CitySampleEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.Add("CitySample");
		ExtraModuleNames.Add("CitySampleEditor");
		ExtraModuleNames.Add("CitySampleAnimGraphRuntime");

		if (Type == TargetType.Editor && Platform.IsInGroup(UnrealPlatformGroup.Linux) && LinuxPlatform.bEnableThreadSanitizer)
		{
			string[] TSanDisabledPlugins = 
			{
				"NeuralNetworkInference",
				"RemoteControl",
				"Text3D",
			};

			foreach (string PluginName in TSanDisabledPlugins)
			{
				DisablePlugins.Add(PluginName);
				EnablePlugins.Remove(PluginName);
			}
		}
	}
}
