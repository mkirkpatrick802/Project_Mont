// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandscapeChunkKey.h"

class FVoxelLandscapeVolumeMesh;

DECLARE_VOXEL_MEMORY_STAT(VOXELLANDSCAPE_API, STAT_VoxelLandscapeVolumeMeshDataMemory, "Voxel Landscape Volume Mesh Data Memory");

class FVoxelLandscapeVolumeMeshData
{
public:
	const FVoxelLandscapeChunkKey ChunkKey;
	const int32 ChunkSize;

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelLandscapeVolumeMeshDataMemory);

	FVoxelLandscapeVolumeMeshData(
		const FVoxelLandscapeChunkKey& ChunkKey,
		const FVoxelLandscapeState& State);

	int64 GetAllocatedSize() const;

public:
	struct FCell
	{
		uint8 X = 0;
		uint8 Y = 0;
		uint8 Z = 0;
		uint8 NumTriangles = 0;
		int32 FirstTriangle = 0;
	};
	struct FTransitionIndex
	{
		uint32 bIsRelative : 1;
		uint32 Index : 31;
	};
	struct FTransitionVertex
	{
		FVector3f Position;
		int32 SourceVertex = 0;
	};
	struct FTextureCoordinate
	{
		uint16 X = 0;
		uint16 Y = 0;
	};

	checkStatic(sizeof(FCell) == 8);
	checkStatic(sizeof(FTransitionIndex) == 4);
	checkStatic(sizeof(FTransitionVertex) == 16);
	checkStatic(sizeof(FTextureCoordinate) == 4);

	int32 NumEdgeVertices = 0;

	TVoxelArray<FCell> Cells;
	TVoxelArray<int32> TriangleToCellIndex;
	TVoxelArray<uint8> CellIndexToDirection;
	TVoxelArray<FTextureCoordinate> CellIndexToTextureCoordinate;

	TVoxelArray<int32> Indices;
	TVoxelArray<FVector3f> Vertices;
	TVoxelArray<FVector3f> VertexNormals;

	TVoxelStaticArray<TVoxelArray<FTransitionIndex>, 6> TransitionIndices;
	TVoxelStaticArray<TVoxelArray<FTransitionVertex>, 6> TransitionVertices;
	TVoxelStaticArray<TVoxelArray<int32>, 6> TransitionTriangleToCellIndex;

	void Shrink();
	TSharedRef<FVoxelLandscapeVolumeMesh> CreateMesh(uint8 TransitionMask) const;
};