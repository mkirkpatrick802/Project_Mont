// Copyright Voxel Plugin SAS. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class VoxelCoreAssets : ModuleRules_Voxel
{
    public VoxelCoreAssets(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "MeshDescription",
			}
		);
	}
}