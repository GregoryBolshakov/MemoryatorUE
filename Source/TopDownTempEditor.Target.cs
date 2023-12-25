using UnrealBuildTool;
using System.Collections.Generic;

public class TopDownTempEditorTarget : TargetRules
{
	public TopDownTempEditorTarget(TargetInfo Target) : base(Target)
	{
		//BuildEnvironment = TargetBuildEnvironment.Unique;
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.Add("TopDownTemp");
	}
}
