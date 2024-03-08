// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelInstancedMeshDataBase.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "VoxelInstancedMeshComponent.generated.h"

DECLARE_VOXEL_MEMORY_STAT(VOXELGRAPHCORE_API, STAT_VoxelInstancedMeshDataMemory, "Voxel Instanced Mesh Data Memory");
DECLARE_VOXEL_COUNTER(VOXELGRAPHCORE_API, STAT_VoxelInstancedMeshNumInstances, "Num Instanced Mesh Instances");

struct VOXELGRAPHCORE_API FVoxelInstancedMeshData : public FVoxelInstancedMeshDataBase
{
	using FVoxelInstancedMeshDataBase::FVoxelInstancedMeshDataBase;

	int32 NumNewInstances = 0;
	TVoxelArray<int32> InstanceIndices;

	TCompatibleVoxelArray<FInstancedStaticMeshInstanceData> PerInstanceSMData;
	TCompatibleVoxelArray<float> PerInstanceSMCustomData;

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelInstancedMeshDataMemory);

	void Build();
	int64 GetAllocatedSize() const;
};

UCLASS()
class VOXELGRAPHCORE_API UVoxelInstancedMeshComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	UVoxelInstancedMeshComponent();

	void AddMeshData(const TSharedRef<FVoxelInstancedMeshData>& NewMeshData);
	void SetMeshData(const TSharedRef<FVoxelInstancedMeshData>& NewMeshData);
	void RemoveInstancesFast(TConstVoxelArrayView<int32> Indices);
	void ReturnToPool();

	void HideInstances(TConstVoxelArrayView<int32> Indices);
	void ShowInstances(TConstVoxelArrayView<int32> Indices);

	virtual void ClearInstances() override;
	virtual void DestroyComponent(bool bPromoteChildren = false) override;

	FORCEINLINE bool HasMeshData() const
	{
		return MeshData.IsValid();
	}

private:
	VOXEL_COUNTER_HELPER(STAT_VoxelInstancedMeshNumInstances, NumInstances);

	TSharedPtr<FVoxelInstancedMeshData> MeshData;
};