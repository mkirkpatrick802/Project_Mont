using UnrealBuildTool;

public class Project_Mont : ModuleRules
{
	public Project_Mont(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] { "TPP_Boilerplate", "VoxelGraphCore"});
	}
}
