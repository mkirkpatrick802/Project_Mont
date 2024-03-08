// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandscapeChunkKey.h"

class FVoxelLandscapeState;
class FVoxelLandscapeVolumeVertexFactoryBase;
class FCardRepresentationData;
class FDistanceFieldVolumeData;

DECLARE_VOXEL_MEMORY_STAT(VOXELLANDSCAPE_API, STAT_VoxelLandscapeVolumeMeshGpuMemory, "Voxel Landscape Volume Mesh Memory (GPU)");

class FVoxelLandscapeVolumeMesh : public TSharedFromThis<FVoxelLandscapeVolumeMesh>
{
public:
	const FVoxelLandscapeChunkKey ChunkKey;

	static TSharedRef<FVoxelLandscapeVolumeMesh> New(
		const FVoxelLandscapeChunkKey& ChunkKey,
		TVoxelArray<int32>&& Indices,
		TVoxelArray<FVector4f>&& Vertices,
		TVoxelArray<FVector3f>&& VertexNormals);

	UE_NONCOPYABLE(FVoxelLandscapeVolumeMesh);
	~FVoxelLandscapeVolumeMesh();

public:
	VOXEL_ALLOCATED_SIZE_GPU_TRACKER(STAT_VoxelLandscapeVolumeMeshGpuMemory);

	int64 GetGpuAllocatedSize() const;

public:
	void Initialize_RenderThread(
		FRHICommandListBase& RHICmdList,
		const FVoxelLandscapeState& State);

private:
	TSharedRef<FVoxelMaterialRef> Material = FVoxelMaterialRef::Default();
	FVoxelBox Bounds;
	TVoxelArray<int32> Indices;
	TVoxelArray<FVector4f> Vertices;
	TVoxelArray<FVector3f> VertexNormals;

	// TODO
	TSharedPtr<FCardRepresentationData> CardRepresentationData;
	TSharedPtr<FDistanceFieldVolumeData> DistanceFieldVolumeData;
	float SelfShadowBias = 0.f;

private:
	int32 NumIndices = 0;
	TSharedPtr<FIndexBuffer> IndicesBuffer;
	TSharedPtr<FVertexBuffer> VerticesBuffer;
	TSharedPtr<FVertexBuffer> VertexNormalsBuffer;
	TSharedPtr<FVoxelLandscapeVolumeVertexFactoryBase> VertexFactory;

	explicit FVoxelLandscapeVolumeMesh(const FVoxelLandscapeChunkKey& ChunkKey)
		: ChunkKey(ChunkKey)
	{
	}

	friend class UVoxelLandscapeVolumeComponent;
	friend class FVoxelLandscapeVolumeSceneProxy;
};