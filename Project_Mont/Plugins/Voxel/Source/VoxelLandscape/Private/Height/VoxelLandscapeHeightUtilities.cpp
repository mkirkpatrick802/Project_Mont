// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Height/VoxelLandscapeHeightUtilities.h"
#include "Height/VoxelLandscapeHeightmapInterpolation.h"
#include "VoxelLandscapeHeightUtilitiesImpl.ispc.generated.h"

void FVoxelLandscapeHeightUtilities::ApplyHeightmap(TArgs<uint8> Args)
{
	VOXEL_FUNCTION_COUNTER_NUM(Args.Span.Size().X * Args.Span.Size().Y, 0);

	Args.LocalScaleZ *= 1. / MAX_uint8;

	switch (Args.Interpolation)
	{
	default: VOXEL_ASSUME(false);
	case EVoxelLandscapeHeightmapInterpolation::NearestNeighbor:
	{
		ispc::VoxelLandscapeHeightUtilities_ApplyHeightmap_NearestNeighbor_uint8(ReinterpretCastRef<ispc::FArgs_uint8>(Args));
	}
	break;
	case EVoxelLandscapeHeightmapInterpolation::Bilinear:
	{
		ispc::VoxelLandscapeHeightUtilities_ApplyHeightmap_Bilinear_uint8(ReinterpretCastRef<ispc::FArgs_uint8>(Args));
	}
	break;
	case EVoxelLandscapeHeightmapInterpolation::Bicubic:
	{
		ispc::VoxelLandscapeHeightUtilities_ApplyHeightmap_Bicubic_uint8(ReinterpretCastRef<ispc::FArgs_uint8>(Args));
	}
	break;
	}
}

void FVoxelLandscapeHeightUtilities::ApplyHeightmap(TArgs<uint16> Args)
{
	VOXEL_FUNCTION_COUNTER_NUM(Args.Span.Size().X * Args.Span.Size().Y, 0);

	Args.LocalScaleZ *= 1. / MAX_uint16;

	switch (Args.Interpolation)
	{
	default: VOXEL_ASSUME(false);
	case EVoxelLandscapeHeightmapInterpolation::NearestNeighbor:
	{
		ispc::VoxelLandscapeHeightUtilities_ApplyHeightmap_NearestNeighbor_uint16(ReinterpretCastRef<ispc::FArgs_uint16>(Args));
	}
	break;
	case EVoxelLandscapeHeightmapInterpolation::Bilinear:
	{
		ispc::VoxelLandscapeHeightUtilities_ApplyHeightmap_Bilinear_uint16(ReinterpretCastRef<ispc::FArgs_uint16>(Args));
	}
	break;
	case EVoxelLandscapeHeightmapInterpolation::Bicubic:
	{
		ispc::VoxelLandscapeHeightUtilities_ApplyHeightmap_Bicubic_uint16(ReinterpretCastRef<ispc::FArgs_uint16>(Args));
	}
	break;
	}
}

void FVoxelLandscapeHeightUtilities::ApplyHeightmap(TArgs<float> Args)
{
	VOXEL_FUNCTION_COUNTER_NUM(Args.Span.Size().X * Args.Span.Size().Y, 0);

	switch (Args.Interpolation)
	{
	default: VOXEL_ASSUME(false);
	case EVoxelLandscapeHeightmapInterpolation::NearestNeighbor:
	{
		ispc::VoxelLandscapeHeightUtilities_ApplyHeightmap_NearestNeighbor_float(ReinterpretCastRef<ispc::FArgs_float>(Args));
	}
	break;
	case EVoxelLandscapeHeightmapInterpolation::Bilinear:
	{
		ispc::VoxelLandscapeHeightUtilities_ApplyHeightmap_Bilinear_float(ReinterpretCastRef<ispc::FArgs_float>(Args));
	}
	break;
	case EVoxelLandscapeHeightmapInterpolation::Bicubic:
	{
		ispc::VoxelLandscapeHeightUtilities_ApplyHeightmap_Bicubic_float(ReinterpretCastRef<ispc::FArgs_float>(Args));
	}
	break;
	}
}

void FVoxelLandscapeHeightUtilities::ComputeNormals(
	const TConstVoxelArrayView<float> Heights,
	const int32 HeightsSize,
	const TVoxelArrayView<FVoxelOctahedron> OutNormals,
	const int32 NormalsSize,
	const float StepZ)
{
	VOXEL_FUNCTION_COUNTER_NUM(NormalsSize * NormalsSize, 1);
	check(Heights.Num() == HeightsSize * HeightsSize);
	check(OutNormals.Num() == NormalsSize * NormalsSize);
	check(HeightsSize == NormalsSize + 2);

	ispc::VoxelLandscapeHeightUtilities_ComputeNormals(
		Heights.GetData(),
		HeightsSize,
		ReinterpretCastPtr<uint16>(OutNormals.GetData()),
		NormalsSize,
		StepZ);
}