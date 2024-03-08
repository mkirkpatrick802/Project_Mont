// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBounds.h"
#include "VoxelSurface.h"
#include "VoxelFunctionLibrary.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "Material/VoxelMaterialBlending.h"
#include "VoxelSurfaceFunctionLibrary.generated.h"

struct FVoxelDetailTextureRef;

UENUM()
enum class EVoxelTransformSpace : uint8
{
	// Transform space of the actor of this graph
	// Typically, the current brush transform
	Local,
	// World space transform in unreal units
	World,
	// Transform space of the actor making the query
	// Typically, the voxel actor which is rendering the mesh
	Query
};

UCLASS()
class VOXELGRAPHCORE_API UVoxelSurfaceFunctionLibrary : public UVoxelFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = "Surface")
	FVoxelBox GetSurfaceBounds(const FVoxelSurface& Surface) const;

	UFUNCTION(Category = "Surface", meta = (ShowInShortList))
	FVoxelBounds MakeBoundsFromLocalBox(const FVoxelBox& Box) const;

	UFUNCTION(Category = "Surface")
	FVoxelBox GetBoundsBox(
		const FVoxelBounds& Bounds,
		EVoxelTransformSpace TransformSpace = EVoxelTransformSpace::Local) const;

	UFUNCTION(Category = "Surface")
	FVoxelMaterialBlendingBuffer BlendMaterials(
		const FVoxelMaterialBlendingBuffer& A,
		const FVoxelMaterialBlendingBuffer& B,
		const FVoxelFloatBuffer& Alpha) const;
};