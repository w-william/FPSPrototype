// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FPSPrototype : ModuleRules
{
	public FPSPrototype(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
            "GameplayAbilities", "GameplayTags", "GameplayTasks",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"FPSPrototype",
			"FPSPrototype/Variant_Horror",
			"FPSPrototype/Variant_Horror/UI",
			"FPSPrototype/Variant_Shooter",
			"FPSPrototype/Variant_Shooter/AI",
			"FPSPrototype/Variant_Shooter/UI",
			"FPSPrototype/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
