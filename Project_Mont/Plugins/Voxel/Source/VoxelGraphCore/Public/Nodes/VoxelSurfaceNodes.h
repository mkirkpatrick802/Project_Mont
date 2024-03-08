// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelSurface.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "VoxelSurfaceNodes.generated.h"

// Invert a surface, ie turn it inside out
USTRUCT(Category = "Surface", meta = (ShowInShortList))
struct VOXELGRAPHCORE_API FVoxelNode_InvertSurface : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelSurface, Surface, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelSurface, NewSurface);
};

// Intersect a surface and a distance field
USTRUCT(Category = "Surface", meta = (ShowInShortList))
struct VOXELGRAPHCORE_API FVoxelNode_MaskSurface : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelSurface, Surface, nullptr);
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, MaskDistance, nullptr);
	VOXEL_INPUT_PIN(float, Smoothness, 100.f);
	VOXEL_OUTPUT_PIN(FVoxelSurface, NewSurface);
};

// Inflate a surface by a constant amount
USTRUCT(Category = "Surface", meta = (ShowInShortList))
struct VOXELGRAPHCORE_API FVoxelNode_InflateSurface : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelSurface, Surface, nullptr);
	VOXEL_INPUT_PIN(float, Amount, 100.f);
	VOXEL_OUTPUT_PIN(FVoxelSurface, NewSurface);
};

// Displace a surface by some amount
// Useful to eg add noise to a surface
USTRUCT(Category = "Surface", meta = (ShowInShortList, Keywords = "shift"))
struct VOXELGRAPHCORE_API FVoxelNode_DisplaceSurface : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelSurface, Surface, nullptr);
	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Displacement, nullptr);
	// Max amount to displace by, used to compute bounds. Displacement will be clamped between -MaxDisplacement and +MaxDisplacement
	// Will increase bounds, so try to keep small
	VOXEL_INPUT_PIN(float, MaxDisplacement, 1000.f);
	VOXEL_OUTPUT_PIN(FVoxelSurface, NewSurface);
};