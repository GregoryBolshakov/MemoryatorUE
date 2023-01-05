// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TopDownTemp : ModuleRules
{
	public TopDownTemp(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Paper2D", "HeadMountedDisplay", "NavigationSystem", "AIModule" });
        PrivateDependencyModuleNames.AddRange(new string[] { "NakamaUnreal", "NakamaCore", "GoogleTest", "CADKernel" });
    }
}
