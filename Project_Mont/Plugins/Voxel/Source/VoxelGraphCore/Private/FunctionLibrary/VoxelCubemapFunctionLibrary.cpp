// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "FunctionLibrary/VoxelCubemapFunctionLibrary.h"
#include "VoxelGraphMigration.h"
#include "VoxelPositionQueryParameter.h"
#include "VoxelCubemapFunctionLibraryImpl.ispc.generated.h"

VOXEL_RUN_ON_STARTUP_GAME(VoxelCubemapFunctionLibraryMigration)
{
	REGISTER_VOXEL_FUNCTION_MIGRATION("Make Cubemap Planet Surface", UVoxelCubemapFunctionLibrary, CreateCubemapPlanetDistanceField);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFloatBuffer UVoxelCubemapFunctionLibrary::CreateCubemapPlanetDistanceField(
	FVoxelBox& Bounds,
	const FVoxelHeightmapRef& PosX,
	const FVoxelHeightmapRef& NegX,
	const FVoxelHeightmapRef& PosY,
	const FVoxelHeightmapRef& NegY,
	const FVoxelHeightmapRef& PosZ,
	const FVoxelHeightmapRef& NegZ,
	const FVector& PlanetCenter,
	const float PlanetRadius,
	const float MaxHeight,
	const EVoxelHeightmapInterpolationType Interpolation,
	const float BicubicSmoothness) const
{
	VOXEL_FUNCTION_COUNTER();

	if (!PosX.Data)
	{
		VOXEL_MESSAGE(Error, "{0}: PosX is null", this);
		return {};
	}

	const FIntPoint Size = PosX.Data->GetSize();

#define CHECK(Name) \
	if (!Name.Data) \
	{ \
		VOXEL_MESSAGE(Error, "{0}: " #Name " is null", this); \
		return {}; \
	} \
	if (Name.Data->GetSize() != Size) \
	{ \
		VOXEL_MESSAGE(Error, "{0}: {1}.Size is different from {2}.Size: {3} != {4}", \
			this, \
			Name.Asset, \
			PosX.Asset, \
			Name.Data->GetSize().ToString(), \
			Size.ToString()); \
		return {}; \
	}

	CHECK(PosY);
	CHECK(PosZ);
	CHECK(NegX);
	CHECK(NegY);
	CHECK(NegZ);

#undef CHECK

	Bounds = FVoxelBox().Extend(PlanetRadius + MaxHeight).ShiftBy(PlanetCenter);

	FindOptionalVoxelQueryParameter_Function(FVoxelPositionQueryParameter, PositionQueryParameter);
	if (!PositionQueryParameter)
	{
		return {};
	}

	const FVoxelVectorBuffer Positions = PositionQueryParameter->GetPositions();

	FVoxelFloatBufferStorage Distance;
	Distance.Allocate(Positions.Num());

	const TVoxelStaticArray<const uint16*, 6> Heightmaps
	{
		PosX.Data->GetHeights().GetData(),
		NegX.Data->GetHeights().GetData(),
		PosY.Data->GetHeights().GetData(),
		NegY.Data->GetHeights().GetData(),
		PosZ.Data->GetHeights().GetData(),
		NegZ.Data->GetHeights().GetData(),
	};

	ForeachVoxelBufferChunk_Parallel(Positions.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		switch (Interpolation)
		{
		default: ensure(false);
		case EVoxelHeightmapInterpolationType::NearestNeighbor:
		case EVoxelHeightmapInterpolationType::Bilinear:
		{
			ispc::VoxelCubemapFunctionLibrary_MakeCubemapPlanetDistanceField_Bilinear(
				Positions.X.GetData(Iterator),
				Positions.X.IsConstant(),
				Positions.Y.GetData(Iterator),
				Positions.Y.IsConstant(),
				Positions.Z.GetData(Iterator),
				Positions.Z.IsConstant(),
				GetISPCValue(PlanetCenter),
				PlanetRadius,
				MaxHeight,
				Size.X,
				Size.Y,
				Heightmaps.GetData(),
				Iterator.Num(),
				Distance.GetData(Iterator));
		}
		break;
		case EVoxelHeightmapInterpolationType::Bicubic:
		{
			ispc::VoxelCubemapFunctionLibrary_MakeCubemapPlanetDistanceField_Bicubic(
				Positions.X.GetData(Iterator),
				Positions.X.IsConstant(),
				Positions.Y.GetData(Iterator),
				Positions.Y.IsConstant(),
				Positions.Z.GetData(Iterator),
				Positions.Z.IsConstant(),
				GetISPCValue(PlanetCenter),
				PlanetRadius,
				MaxHeight,
				BicubicSmoothness,
				Size.X,
				Size.Y,
				Heightmaps.GetData(),
				Iterator.Num(),
				Distance.GetData(Iterator));
		}
		break;
		}
	});

	return FVoxelFloatBuffer::Make(Distance);
}