// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WorldAudioDataSystem : ModuleRules
{
	public WorldAudioDataSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Soundscape",
				"PointCloud",
				"DeveloperSettings",
				"GameplayTags",
				"MassEntity",
				"MassSpawner",
				"StructUtils",
				"AudioModulation",
				"Crossfader",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"MassCommon",
				"MassTraffic",
				"MassCrowd",
				"MassMovement",
				"MassLOD",
				"MassRepresentation",
				"MetasoundEngine",
				"AudioMixer",
				"MassNavigation",
				"AudioExtensions"
				// ... add private dependencies that you statically link with here ...	
			}
			);

		// Because of UEdGraph's existence in the Engine Module & being the parent
		// class of MetasoundEditorGraph (referenced by UMetaSound... types), editor
		// class definitions must be loaded prior to the WorldDataGameplayActor to ensure
		// references are properly loaded.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"MetasoundEditor",
			}
			);
		}

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
