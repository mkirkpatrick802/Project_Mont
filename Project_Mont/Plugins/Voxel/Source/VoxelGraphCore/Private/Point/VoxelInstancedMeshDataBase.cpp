// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Point/VoxelInstancedMeshDataBase.h"
#include "VoxelAABBTree.h"

int64 FVoxelInstancedMeshDataBase::GetAllocatedSize() const
{
	int64 AllocatedSize = 0;
	AllocatedSize += Transforms.GetAllocatedSize();
	AllocatedSize += PointIds_Transient.GetAllocatedSize();
	AllocatedSize += CustomDatas_Transient.GetAllocatedSize();

	for (const TVoxelArray<float>& CustomData : CustomDatas_Transient)
	{
		AllocatedSize += CustomData.GetAllocatedSize();
	}

	return AllocatedSize;
}