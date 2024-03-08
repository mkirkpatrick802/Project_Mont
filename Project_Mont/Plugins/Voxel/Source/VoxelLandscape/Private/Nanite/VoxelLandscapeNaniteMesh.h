// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandscapeChunkKey.h"

class FStaticMeshRenderData;

class VOXELLANDSCAPE_API FVoxelLandscapeNaniteMesh : public TSharedFromThis<FVoxelLandscapeNaniteMesh>
{
public:
	const FVoxelLandscapeChunkKey ChunkKey;
	bool bIsEmpty = false;
	TUniquePtr<FStaticMeshRenderData> RenderData;

	explicit FVoxelLandscapeNaniteMesh(const FVoxelLandscapeChunkKey& ChunkKey);
	~FVoxelLandscapeNaniteMesh();
	UE_NONCOPYABLE(FVoxelLandscapeNaniteMesh);

	UStaticMesh* GetStaticMesh();

private:
	TWeakObjectPtr<UStaticMesh> WeakStaticMesh;
};