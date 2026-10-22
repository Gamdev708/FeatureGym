// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FeatureGym2 : ModuleRules
{
	public FeatureGym2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate", "AdvancedParkourSystem", "ComboSystemPlugin", "MotionWarping"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"FeatureGym2",
			"FeatureGym2/Variant_Platforming",
			"FeatureGym2/Variant_Platforming/Animation",
			"FeatureGym2/Variant_Combat",
			"FeatureGym2/Variant_Combat/AI",
			"FeatureGym2/Variant_Combat/Animation",
			"FeatureGym2/Variant_Combat/Gameplay",
			"FeatureGym2/Variant_Combat/Interfaces",
			"FeatureGym2/Variant_Combat/UI",
			"FeatureGym2/Variant_SideScrolling",
			"FeatureGym2/Variant_SideScrolling/AI",
			"FeatureGym2/Variant_SideScrolling/Gameplay",
			"FeatureGym2/Variant_SideScrolling/Interfaces",
			"FeatureGym2/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "AdvancedParkourSystem", "ComboSystemPlugin", "MotionWarping", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
