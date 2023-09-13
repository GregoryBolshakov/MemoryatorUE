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
			});
		PrivateDependencyModuleNames.AddRange(new string[] { "GoogleTest", "CADKernel" });
	}
}
