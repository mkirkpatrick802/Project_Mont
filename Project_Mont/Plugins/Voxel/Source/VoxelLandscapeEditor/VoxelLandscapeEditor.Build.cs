// Copyright Voxel Plugin SAS. All Rights Reserved.

using UnrealBuildTool;

public class VoxelLandscapeEditor : ModuleRules_Voxel
{
    public VoxelLandscapeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "VoxelLandscape",
            }
        );
    }
}