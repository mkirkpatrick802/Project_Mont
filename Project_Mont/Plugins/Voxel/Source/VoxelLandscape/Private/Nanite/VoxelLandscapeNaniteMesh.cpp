// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nanite/VoxelLandscapeNaniteMesh.h"
#include "VoxelNaniteBuilder.h"
#include "Engine/StaticMesh.h"

FVoxelLandscapeNaniteMesh::FVoxelLandscapeNaniteMesh(const FVoxelLandscapeChunkKey& ChunkKey)
	: ChunkKey(ChunkKey)
{
}

FVoxelLandscapeNaniteMesh::~FVoxelLandscapeNaniteMesh()
{
	if (WeakStaticMesh.IsExplicitlyNull())
	{
		return;
	}

	RunOnGameThread([WeakStaticMesh = WeakStaticMesh]
	{
		UStaticMesh* StaticMesh = WeakStaticMesh.Get();
		if (!StaticMesh)
		{
			return;
		}

		StaticMesh->ReleaseResources();
	});
}

UStaticMesh* FVoxelLandscapeNaniteMesh::GetStaticMesh()
{
	VOXEL_FUNCTION_COUNTER();

	if (UStaticMesh* StaticMesh = WeakStaticMesh.Get())
	{
		return StaticMesh;
	}

	if (!ensure(RenderData))
	{
		return nullptr;
	}

	UStaticMesh* StaticMesh = FVoxelNaniteBuilder::CreateStaticMesh(MoveTemp(RenderData));
	WeakStaticMesh = StaticMesh;
	return StaticMesh;
}