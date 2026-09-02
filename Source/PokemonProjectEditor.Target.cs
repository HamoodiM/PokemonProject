using UnrealBuildTool;
using System.Collections.Generic;

public class PokemonProjectEditorTarget : TargetRules
{
	public PokemonProjectEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		bOverrideBuildEnvironment = true;

		ExtraModuleNames.AddRange(new string[] { "PokemonProject" });
	}
}
