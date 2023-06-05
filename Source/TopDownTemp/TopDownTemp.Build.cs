// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TopDownTemp : ModuleRules
{
	public TopDownTemp(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"AIModule",
				"Core",
				"CoreUObject",
				"Engine",
				"HeadMountedDisplay",
				"NavigationSystem",
				"InputCore",
				"NakamaManager",
				"NakamaUnreal",
				"NakamaCore",
				"OnlineSubsystemSteam",
				"OnlineSubsystem",
				"OnlineSubsystemUtils",
				"Paper2D",
				"Steamworks",
				"UMG",
			});
		PrivateDependencyModuleNames.AddRange(new string[] { "GoogleTest", "CADKernel" });
	}
}
