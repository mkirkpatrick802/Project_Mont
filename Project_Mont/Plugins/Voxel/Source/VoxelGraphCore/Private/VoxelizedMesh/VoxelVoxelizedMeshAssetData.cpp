// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelizedMesh/VoxelVoxelizedMeshAssetData.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshAsset.h"
#include "VoxelDependency.h"

TSharedRef<FVoxelVoxelizedMeshAssetData> FVoxelVoxelizedMeshAssetData::Create(UVoxelVoxelizedMeshAsset& Asset)
{
	check(IsInGameThread());

	static TVoxelMap<FObjectKey, TWeakPtr<FVoxelVoxelizedMeshAssetData>> AssetToWeakData = INLINE_LAMBDA -> TVoxelMap<FObjectKey, TWeakPtr<FVoxelVoxelizedMeshAssetData>>
	{
		GOnVoxelModuleUnloaded_DoCleanup.AddLambda([]
		{
			AssetToWeakData.Empty();
		});
		return {};
	};

	TWeakPtr<FVoxelVoxelizedMeshAssetData>& WeakData = AssetToWeakData.FindOrAdd(&Asset);

	TSharedPtr<FVoxelVoxelizedMeshAssetData> Data = WeakData.Pin();
	if (!Data)
	{
		Data = MakeVoxelShareable(new (GVoxelMemory) FVoxelVoxelizedMeshAssetData(Asset));
		WeakData = Data;

		Data->SetMeshData(Asset.GetData());
		Asset.OnDataChanged.Add(MakeWeakPtrDelegate(Data, [&Data = *Data]
		{
			if (!ensure(Data.WeakAsset.IsValid()))
			{
				return;
			}

			Data.SetMeshData(Data.WeakAsset->GetData());
		}));
	}
	return Data.ToSharedRef();
}

TSharedPtr<const FVoxelVoxelizedMeshData> FVoxelVoxelizedMeshAssetData::GetMeshData() const
{
	VOXEL_SCOPE_LOCK(CriticalSection);
	return MeshData_RequiresLock;
}

void FVoxelVoxelizedMeshAssetData::SetMeshData(const TSharedPtr<const FVoxelVoxelizedMeshData>& NewMeshData)
{
	{
		VOXEL_SCOPE_LOCK(CriticalSection);
		MeshData_RequiresLock = NewMeshData;
	}

	Dependency->Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelVoxelizedMeshAssetData::FVoxelVoxelizedMeshAssetData(UVoxelVoxelizedMeshAsset& Asset)
	: Dependency(FVoxelDependency::Create(STATIC_FNAME("VoxelizedMesh"), Asset.GetFName()))
	, WeakAsset(&Asset)
{
}