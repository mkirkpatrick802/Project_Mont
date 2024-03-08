// Copyright Voxel Plugin SAS. All Rights Reserved.

using UnrealBuildTool;

public class VoxelCubeEditor : ModuleRules_Voxel
{
    public VoxelCubeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "VoxelCube",
                "VoxelGraphEditor",
            }
        );
    }
}