using UnrealBuildTool;

public class NakamaManager : ModuleRules
{
	public NakamaManager(ReadOnlyTargetRules Target) : base(Target)
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
				// ... add other public dependencies that you statically link with here ...
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Json",
				"JsonUtilities",
				"Slate",
				"SlateCore",
				"NakamaUnreal",
				"NakamaCore"
				// ... add private dependencies that you statically link with here ...	
			}
			);

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PrivateDependencyModuleNames.Add("OnlineSubsystem");
			PrivateDependencyModuleNames.Add("OnlineSubsystemSteam");
			PrivateDependencyModuleNames.Add("OnlineSubsystemUtils");
			PrivateDependencyModuleNames.Add("Steamworks");
		}

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
