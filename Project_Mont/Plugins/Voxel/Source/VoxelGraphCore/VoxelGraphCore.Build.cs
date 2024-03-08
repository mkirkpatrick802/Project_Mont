// Copyright Voxel Plugin SAS. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class VoxelGraphCore : ModuleRules_Voxel
{
    public VoxelGraphCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"TraceLog",
			    "Chaos",
				"Slate",
				"SlateCore",
			    "PhysicsCore",
			}
		);

		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"MessageLog",
				}
			);
		}
	}
}