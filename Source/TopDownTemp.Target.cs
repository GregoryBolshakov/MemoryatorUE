using UnrealBuildTool;
using System.Collections.Generic;

public class TopDownTempTarget : TargetRules
{
	public TopDownTempTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		//bUsesSteam = true;
		ExtraModuleNames.Add("TopDownTemp");
	}
}
