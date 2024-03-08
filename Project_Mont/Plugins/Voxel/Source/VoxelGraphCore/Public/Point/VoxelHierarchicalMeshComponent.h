// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelInstancedMeshDataBase.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "VoxelHierarchicalMeshComponent.generated.h"

DECLARE_VOXEL_MEMORY_STAT(VOXELGRAPHCORE_API, STAT_VoxelHierarchicalMeshDataMemory, "Voxel Hierarchical Mesh Data Memory");
DECLARE_VOXEL_MEMORY_STAT(VOXELGRAPHCORE_API, STAT_VoxelHierarchicalMeshMemory, "Voxel Hierarchical Mesh Memory");
DECLARE_VOXEL_COUNTER(VOXELGRAPHCORE_API, STAT_VoxelHierarchicalMeshNumInstances, "Num Hierarchical Mesh Instances");

struct VOXELGRAPHCORE_API FVoxelHierarchicalMeshBuiltData
{
	TUniquePtr<FStaticMeshInstanceData> InstanceBuffer;
	TArray<FClusterNode> ClusterTree;
	int32 OcclusionLayerNum = 0;

	TCompatibleVoxelArray<FInstancedStaticMeshInstanceData> InstanceDatas;
	TCompatibleVoxelArray<float> CustomDatas;
	TCompatibleVoxelArray<int32> InstanceReorderTable;
};

struct VOXELGRAPHCORE_API FVoxelHierarchicalMeshData : public FVoxelInstancedMeshDataBase
{
	using FVoxelInstancedMeshDataBase::FVoxelInstancedMeshDataBase;

	mutable TSharedPtr<FVoxelHierarchicalMeshBuiltData> BuiltData;

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelHierarchicalMeshDataMemory);

	void Build();
	int64 GetAllocatedSize() const;
};

UCLASS()
class VOXELGRAPHCORE_API UVoxelHierarchicalMeshComponent : public UHierarchicalInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	UVoxelHierarchicalMeshComponent();

	TSharedPtr<const FVoxelHierarchicalMeshData> GetMeshData() const
	{
		return MeshData;
	}

	void SetMeshData(const TSharedRef<const FVoxelHierarchicalMeshData>& NewMeshData);
	void RemoveInstancesFast(TConstVoxelArrayView<int32> Indices);
	void ReturnToPool();

	void HideInstances(TConstVoxelArrayView<int32> Indices);
	void ShowInstances(TConstVoxelArrayView<int32> Indices);

	int64 GetAllocatedSize() const;
	void ReleasePerInstanceRenderData_Safe();

	virtual void ClearInstances() override;
	virtual void DestroyComponent(bool bPromoteChildren = false) override;

	static void AsyncTreeBuild(
		FVoxelHierarchicalMeshBuiltData& OutBuiltData,
		const FBox& MeshBox,
		int32 InDesiredInstancesPerLeaf,
		const FVoxelHierarchicalMeshData& InMeshData);

private:
	VOXEL_COUNTER_HELPER(STAT_VoxelHierarchicalMeshNumInstances, NumInstances);
	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelHierarchicalMeshMemory);

	TSharedPtr<const FVoxelHierarchicalMeshData> MeshData;

	void SetBuiltData(FVoxelHierarchicalMeshBuiltData&& BuiltData);
};