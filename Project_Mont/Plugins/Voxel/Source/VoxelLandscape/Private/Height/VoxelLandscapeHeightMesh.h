// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandscapeChunkKey.h"

class FVoxelLandscapeState;
class FVoxelLandscapeHeightVertexFactory;
class FCardRepresentationData;
class FDistanceFieldVolumeData;

DECLARE_VOXEL_MEMORY_STAT(VOXELLANDSCAPE_API, STAT_VoxelLandscapeHeightMeshMemory, "Voxel Landscape Height Mesh Memory");
DECLARE_VOXEL_MEMORY_STAT(VOXELLANDSCAPE_API, STAT_VoxelLandscapeHeightMeshGpuMemory, "Voxel Landscape Height Mesh Memory (GPU)");

class VOXELLANDSCAPE_API FVoxelLandscapeHeightMesh : public TSharedFromThis<FVoxelLandscapeHeightMesh>
{
public:
	const FVoxelLandscapeChunkKey ChunkKey;

	static TSharedRef<FVoxelLandscapeHeightMesh> New(
		const FVoxelLandscapeChunkKey& ChunkKey,
		TVoxelArray<float>&& Heights,
		TVoxelArray<FVoxelOctahedron>&& Normals);

	UE_NONCOPYABLE(FVoxelLandscapeHeightMesh);
	~FVoxelLandscapeHeightMesh();

public:
	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelLandscapeHeightMeshMemory);
	VOXEL_ALLOCATED_SIZE_GPU_TRACKER(STAT_VoxelLandscapeHeightMeshGpuMemory);

	int64 GetAllocatedSize() const;
	int64 GetGpuAllocatedSize() const;

public:
	bool ShouldRender() const
	{
		return NumIndices > 0;
	}

	void Initialize_AsyncThread(const FVoxelLandscapeState& State);

	void Initialize_RenderThread(
		FRHICommandListBase& RHICmdList,
		const FVoxelLandscapeState& State);

public:
	// TODO
	TSharedPtr<FCardRepresentationData> CardRepresentationData;
	TSharedPtr<FDistanceFieldVolumeData> DistanceFieldVolumeData;
	float SelfShadowBias = 0.f;

private:
	TSharedRef<FVoxelMaterialRef> Material = FVoxelMaterialRef::Default();
	FVoxelBox Bounds;
	int32 NumIndices = 0;

	TSharedPtr<FIndexBuffer> IndicesBuffer;
	TSharedPtr<FReadBuffer> HeightsBuffer;
	FTextureRHIRef NormalTexture;
	TSharedPtr<FVoxelLandscapeHeightVertexFactory> VertexFactory;

	TVoxelArray<uint32> Indices;
	TVoxelArray<float> Heights;
	TVoxelArray<FVoxelOctahedron> Normals;

	explicit FVoxelLandscapeHeightMesh(const FVoxelLandscapeChunkKey& ChunkKey)
		: ChunkKey(ChunkKey)
	{
	}

	friend class UVoxelLandscapeHeightComponent;
	friend class FVoxelLandscapeHeightSceneProxy;
};