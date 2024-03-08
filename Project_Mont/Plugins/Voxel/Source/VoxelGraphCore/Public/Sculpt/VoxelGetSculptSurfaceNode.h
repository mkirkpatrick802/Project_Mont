// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelSurface.h"
#include "VoxelGetSculptSurfaceNode.generated.h"

class FVoxelSculptStorageData;

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelSculptStorageQueryParameter : public FVoxelQueryParameter
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_PARAMETER_BODY()

	FVoxelTransformRef SurfaceToWorld;
	TSharedPtr<FVoxelSculptStorageData> Data;
	int32 VoxelSize = 0;
	TSharedPtr<const TVoxelComputeValue<FVoxelSurface>> Compute;
};

#if 0 // TODO
USTRUCT()
struct VOXELGRAPHCORE_API FVoxelSculptSurface : public FVoxelForwardingSurface
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVoxelTransformRef SurfaceToWorld;
	TSharedPtr<FVoxelSculptStorageData> Data;
	int32 VoxelSize = 0;

	//~ Begin FVoxelSurface Interface
	virtual FVoxelBounds GetBounds() const override;
	virtual TValue<FVoxelBuffer> Get(FName Name, const FVoxelQuery& Query) const override;
	//~ End FVoxelSurface Interface
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(Category = "Sculpt")
struct VOXELGRAPHCORE_API FVoxelNode_GetSculptSurface : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_OUTPUT_PIN(FVoxelSurface, Surface);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelNode_GetSculptSurface_Distance : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_CALL_PARAM(FVoxelTransformRef, SurfaceToWorld);
	VOXEL_CALL_PARAM(TSharedPtr<FVoxelSculptStorageData>, Data);
	VOXEL_CALL_PARAM(int32, VoxelSize);
	VOXEL_CALL_PARAM(TSharedPtr<const FVoxelSurface>, Surface);

	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Distance);
};
#endif