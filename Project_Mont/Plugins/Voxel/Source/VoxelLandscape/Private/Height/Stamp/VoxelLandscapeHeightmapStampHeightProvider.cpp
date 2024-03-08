// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Height/Stamp/VoxelLandscapeHeightmapStampHeightProvider.h"
#include "Height/Stamp/VoxelLandscapeHeightmapStampAsset.h"
#include "Height/VoxelLandscapeHeightUtilities.h"

bool FVoxelLandscapeHeightmapStampHeightProvider::TryInitialize()
{
	if (!Stamp)
	{
		return false;
	}

	Proxy = MakeVoxelShared<FStampProxy>(*Stamp);
	return true;
}

FVoxelBox2D FVoxelLandscapeHeightmapStampHeightProvider::GetBounds() const
{
	const FVector2D Size
	{
		double(Proxy->StampData->GetSizeX()),
		double(Proxy->StampData->GetSizeY())
	};
	return FVoxelBox2D(-Size / 2, Size / 2).Scale(Proxy->ScaleXY);
}

FVoxelFuture FVoxelLandscapeHeightmapStampHeightProvider::Apply(const FVoxelLandscapeHeightQuery& Query) const
{
	FVoxelLandscapeHeightUtilities::ApplyHeightmap(FVoxelLandscapeHeightUtilities::TArgs<uint16>
	{
		Query,
		Proxy->ScaleXY,
		Proxy->ScaleZ,
		Proxy->OffsetZ,
		Proxy->BicubicSmoothness,
		Proxy->Interpolation,
		Proxy->StampData->GetSizeX(),
		Proxy->StampData->GetSizeY(),
		Proxy->StampData->GetHeights()
	});

	return FVoxelFuture::Done();
}

FVoxelLandscapeHeightmapStampHeightProvider::FStampProxy::FStampProxy(const UVoxelLandscapeHeightmapStampAsset& Asset)
	: Interpolation(Asset.Interpolation)
	, ScaleXY(Asset.ScaleXY)
	, ScaleZ(Asset.GetFinalScaleZ())
	, OffsetZ(Asset.GetFinalOffsetZ())
	, BicubicSmoothness(Asset.BicubicSmoothness)
	, StampData(Asset.GetStampData())
{
}