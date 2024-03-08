// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "Material/VoxelMaterial.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "MarchingCube/VoxelMarchingCubeSurface.h"
#include "VoxelNode_CreateMarchingCubeMesh.generated.h"

class FVoxelMarchingCubeMesh;

USTRUCT()
struct FVoxelMarchingCubeMeshWrapper
{
	GENERATED_BODY()

	TSharedPtr<FVoxelMarchingCubeMesh> Mesh;
};

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelNode_CreateMarchingCubeMesh : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelMarchingCubeSurface, Surface, nullptr);
	// Use for vertex normals & distance field
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Distance, nullptr);
	VOXEL_INPUT_PIN(FVoxelMaterial, Material, nullptr);
	VOXEL_INPUT_PIN(bool, GenerateDistanceField, nullptr);
	VOXEL_INPUT_PIN(float, DistanceFieldBias, nullptr);
	VOXEL_INPUT_PIN(float, DistanceFieldSelfShadowBias, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelMarchingCubeMeshWrapper, Mesh);
};