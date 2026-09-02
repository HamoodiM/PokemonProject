using UnrealBuildTool;
using System.Collections.Generic;

public class PokemonProjectTarget : TargetRules
{
	public PokemonProjectTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange(new string[] { "PokemonProject" });
	}
}
