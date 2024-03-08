// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class FVoxelDependency;
class FVoxelVoxelizedMeshData;
class UVoxelVoxelizedMeshAsset;

class VOXELGRAPHCORE_API FVoxelVoxelizedMeshAssetData : public TSharedFromThis<FVoxelVoxelizedMeshAssetData>
{
public:
	const TSharedRef<FVoxelDependency> Dependency;
	const TWeakObjectPtr<UVoxelVoxelizedMeshAsset> WeakAsset;

	static TSharedRef<FVoxelVoxelizedMeshAssetData> Create(UVoxelVoxelizedMeshAsset& Asset);

	TSharedPtr<const FVoxelVoxelizedMeshData> GetMeshData() const;
	void SetMeshData(const TSharedPtr<const FVoxelVoxelizedMeshData>& NewMeshData);

private:
	mutable FVoxelCriticalSection CriticalSection;
	TSharedPtr<const FVoxelVoxelizedMeshData> MeshData_RequiresLock;

	explicit FVoxelVoxelizedMeshAssetData(UVoxelVoxelizedMeshAsset& Asset);
};