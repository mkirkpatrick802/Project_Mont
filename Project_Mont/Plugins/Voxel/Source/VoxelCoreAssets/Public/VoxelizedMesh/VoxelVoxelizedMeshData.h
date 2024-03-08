// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshSettings.h"

class UVoxelVoxelizedMeshAsset;

DECLARE_VOXEL_MEMORY_STAT(VOXELCOREASSETS_API, STAT_VoxelVoxelizedMeshData, "Voxel Voxelized Mesh Data Memory");

class VOXELCOREASSETS_API FVoxelVoxelizedMeshData
{
public:
	FVoxelBox MeshBounds;
	int32 VoxelSize = 0;
	float MaxSmoothness = 0;
	FVoxelVoxelizedMeshSettings VoxelizerSettings;

	FVector3f Origin = FVector3f::ZeroVector;
	FIntVector Size = FIntVector::ZeroValue;
	TVoxelArray<float> DistanceField;
	TVoxelArray<FVoxelOctahedron> Normals;

	FVoxelVoxelizedMeshData() = default;

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelVoxelizedMeshData);

	int64 GetAllocatedSize() const;
	void Serialize(FArchive& Ar);

	static TSharedPtr<FVoxelVoxelizedMeshData> VoxelizeMesh(
		const UStaticMesh& StaticMesh,
		int32 VoxelSize,
		float MaxSmoothness,
		const FVoxelVoxelizedMeshSettings& VoxelizerSettings);
};