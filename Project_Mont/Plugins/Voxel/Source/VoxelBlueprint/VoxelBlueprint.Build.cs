// Copyright Voxel Plugin SAS. All Rights Reserved.

using UnrealBuildTool;

public class VoxelBlueprint : ModuleRules_Voxel
{
    public VoxelBlueprint(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "VoxelCoreEditor",
                "VoxelGraphCore",
                "VoxelGraphEditor",
                "Slate",
                "SlateCore",
                "ToolMenus",
                "GraphEditor",
                "BlueprintGraph",
                "KismetCompiler",
            }
        );
    }
}