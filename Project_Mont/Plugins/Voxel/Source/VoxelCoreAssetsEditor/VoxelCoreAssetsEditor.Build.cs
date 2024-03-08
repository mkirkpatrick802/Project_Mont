// Copyright Voxel Plugin SAS. All Rights Reserved.

using UnrealBuildTool;

public class VoxelCoreAssetsEditor : ModuleRules_Voxel
{
    public VoxelCoreAssetsEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "VoxelCoreAssets",
                "VoxelGraphCore",
            }
        );
    }
}