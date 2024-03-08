// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Point/VoxelPointId.h"
#include "Point/VoxelPointChunkRef.h"
#include "Buffer/VoxelStaticMeshBuffer.h"

struct VOXELGRAPHCORE_API FVoxelInstancedMeshDataBase
{
public:
	const FVoxelStaticMesh Mesh;
	const FVoxelPointChunkRef ChunkRef;

	FVoxelInstancedMeshDataBase(
		const FVoxelStaticMesh& Mesh,
		const FVoxelPointChunkRef& ChunkRef)
		: Mesh(Mesh)
		, ChunkRef(ChunkRef)
	{
	}

	TVoxelArray<FTransform3f> Transforms;
	FVoxelOptionalBox Bounds;

	TVoxelArray<FVoxelPointId> PointIds_Transient;

	int32 NumCustomDatas = 0;
	TVoxelArray<TVoxelArray<float>> CustomDatas_Transient;

	FORCEINLINE int32 Num() const
	{
		return Transforms.Num();
	}
	int64 GetAllocatedSize() const;
};