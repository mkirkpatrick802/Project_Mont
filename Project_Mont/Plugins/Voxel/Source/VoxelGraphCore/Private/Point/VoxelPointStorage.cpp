// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Point/VoxelPointStorage.h"
#include "Point/VoxelPointStorageData.h"

void UVoxelPointStorage::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER();

	Super::Serialize(Ar);

	if (IsReloadActive())
	{
		return;
	}

	if (!Data)
	{
		Data = MakeVoxelShared<FVoxelPointStorageData>(GetFName());
	}

	FVoxelUtilities::SerializeBulkData(this, BulkData, Ar, [&](FArchive& BulkDataAr)
	{
		Data->Serialize(BulkDataAr);
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelPointStorageData> UVoxelPointStorage::GetData()
{
	if (!Data)
	{
		Data = MakeVoxelShared<FVoxelPointStorageData>(GetFName());
	}
	return Data.ToSharedRef();
}