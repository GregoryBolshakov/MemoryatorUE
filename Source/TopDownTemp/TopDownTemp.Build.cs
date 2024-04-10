using UnrealBuildTool;

public class TopDownTemp : ModuleRules
{
	public TopDownTemp(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		OptimizeCode = CodeOptimization.Never;

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
				"XRBase",
			});
		PrivateDependencyModuleNames.AddRange(new string[] { "GoogleTest", "CADKernel", "XRBase", "PCG", "ReplicationGraph" });

		bool bTargetConfig = Target.Configuration != UnrealTargetConfiguration.Shipping &&
		                     Target.Configuration != UnrealTargetConfiguration.Test;

		if (Target.bBuildDeveloperTools || bTargetConfig)
		{
			PrivateDependencyModuleNames.Add("GameplayDebugger");
			PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=1");
		}
		else
		{
			PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=0");
		}
	}
}
