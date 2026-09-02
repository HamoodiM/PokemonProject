using UnrealBuildTool;

public class PokemonProject : ModuleRules
{
	public PokemonProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "PaperZD" });

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
