// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMarchingCubeCollider.generated.h"

DECLARE_VOXEL_MEMORY_STAT(VOXELGRAPHCORE_API, STAT_VoxelMarchingCubeColliderMemory, "Voxel Marching Cube Collider Memory");

class VOXELGRAPHCORE_API FVoxelMarchingCubeCollider
{
public:
	const FVector Offset;
	const FVoxelBox LocalBounds;
	const UE_504_SWITCH(TSharedRef, TRefCountPtr)<Chaos::FTriangleMeshImplicitObject> TriangleMesh;
	const TVoxelArray<TWeakObjectPtr<UPhysicalMaterial>> PhysicalMaterials;

	FVoxelMarchingCubeCollider(
		const FVector& Offset,
		const FVoxelBox& LocalBounds,
		const UE_504_SWITCH(TSharedRef, TRefCountPtr)<Chaos::FTriangleMeshImplicitObject>& TriangleMesh,
		const TVoxelArray<TWeakObjectPtr<UPhysicalMaterial>>& PhysicalMaterials);
	~FVoxelMarchingCubeCollider();
	UE_NONCOPYABLE(FVoxelMarchingCubeCollider);

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelMarchingCubeColliderMemory);

public:
	int64 GetAllocatedSize() const;

	static TSharedPtr<FVoxelMarchingCubeCollider> Create(
		const FVector& Offset,
		TConstVoxelArrayView<int32> Indices,
		TConstVoxelArrayView<FVector3f> Vertices,
		TConstVoxelArrayView<uint16> FaceMaterials,
		TVoxelArray<TWeakObjectPtr<UPhysicalMaterial>>&& PhysicalMaterials);
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelMarchingCubeColliderWrapper
{
	GENERATED_BODY()

	TSharedPtr<FVoxelMarchingCubeCollider> Collider;
};