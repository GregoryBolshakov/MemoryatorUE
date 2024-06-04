using UnrealBuildTool;
using System.Collections.Generic;

public class TopDownTempEditorTarget : TargetRules
{
	public TopDownTempEditorTarget(TargetInfo Target) : base(Target)
	{
		//BuildEnvironment = TargetBuildEnvironment.Unique;
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.Add("TopDownTemp");
	}
}
