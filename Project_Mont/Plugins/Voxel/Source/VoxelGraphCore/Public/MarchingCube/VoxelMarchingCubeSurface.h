// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMarchingCubeSurface.generated.h"

struct FVoxelMarchingCubeCell
{
	uint8 X = 0;
	uint8 Y = 0;
	uint8 Z = 0;
	uint8 NumTriangles = 0;
	int32 FirstTriangle = 0;
};
checkStatic(sizeof(FVoxelMarchingCubeCell) == 8);

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelMarchingCubeSurface
{
	GENERATED_BODY()

	int32 LOD = 0;
	int32 ChunkSize = 0;
	int32 ScaledVoxelSize = 0;
	FVoxelBox ChunkBounds;

	int32 NumEdgeVertices = 0;

	TVoxelArray<FVoxelMarchingCubeCell> Cells;
	TVoxelArray<int32> Indices;
	TVoxelArray<FVector3f> Vertices;
	TVoxelArray<int32> CellIndices;

	struct FTransitionIndex
	{
		uint32 bIsRelative : 1;
		uint32 Index : 31;
	};
	TVoxelStaticArray<TVoxelArray<FTransitionIndex>, 6> TransitionIndices;

	struct FTransitionVertex
	{
		FVector3f Position;
		int32 SourceVertex = 0;
	};
	TVoxelStaticArray<TVoxelArray<FTransitionVertex>, 6> TransitionVertices;

	TVoxelStaticArray<TVoxelArray<int32>, 6> TransitionCellIndices;
};