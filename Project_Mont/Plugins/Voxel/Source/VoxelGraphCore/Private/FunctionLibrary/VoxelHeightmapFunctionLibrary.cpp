// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "FunctionLibrary/VoxelHeightmapFunctionLibrary.h"
#include "VoxelHeightmapFunctionLibraryImpl.ispc.generated.h"

FVoxelFloatBuffer UVoxelHeightmapFunctionLibrary::SampleHeightmap(
	const FVoxelHeightmapRef& Heightmap,
	const FVoxelVector2DBuffer& Position,
	const EVoxelHeightmapInterpolationType Interpolation,
	const float BicubicSmoothness) const
{
	VOXEL_SCOPE_COUNTER_FORMAT("SampleHeightmap %s Num=%d", *UEnum::GetValueAsString(Interpolation), Position.Num());

	if (!Heightmap.Data)
	{
		VOXEL_MESSAGE(Error, "{0}: Heightmap is null", this);
		return 0.f;
	}
	const FVoxelHeightmapConfig& Config = Heightmap.Config;

	const float ScaleZ = Config.ScaleZ * Config.InternalScaleZ / MAX_uint16;
	const float OffsetZ = Config.ScaleZ * Config.InternalOffsetZ;

	FVoxelFloatBufferStorage Result;
	Result.Allocate(Position.Num());

	ForeachVoxelBufferChunk_Parallel(Position.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		switch (Interpolation)
		{
		default: ensure(false);
		case EVoxelHeightmapInterpolationType::NearestNeighbor:
		{
			ispc::VoxelHeightmapFunctionLibrary_SampleHeightmap_NearestNeighbor(
				Position.X.GetData(Iterator),
				Position.X.IsConstant(),
				Position.Y.GetData(Iterator),
				Position.Y.IsConstant(),
				Config.ScaleXY,
				ScaleZ,
				OffsetZ,
				Heightmap.Data->GetSizeX(),
				Heightmap.Data->GetSizeY(),
				Heightmap.Data->GetHeights().GetData(),
				Iterator.Num(),
				Result.GetData(Iterator));
		}
		break;
		case EVoxelHeightmapInterpolationType::Bilinear:
		{
			ispc::VoxelHeightmapFunctionLibrary_SampleHeightmap_Bilinear(
				Position.X.GetData(Iterator),
				Position.X.IsConstant(),
				Position.Y.GetData(Iterator),
				Position.Y.IsConstant(),
				Config.ScaleXY,
				ScaleZ,
				OffsetZ,
				Heightmap.Data->GetSizeX(),
				Heightmap.Data->GetSizeY(),
				Heightmap.Data->GetHeights().GetData(),
				Iterator.Num(),
				Result.GetData(Iterator));
		}
		break;
		case EVoxelHeightmapInterpolationType::Bicubic:
		{
			ispc::VoxelHeightmapFunctionLibrary_SampleHeightmap_Bicubic(
				Position.X.GetData(Iterator),
				Position.X.IsConstant(),
				Position.Y.GetData(Iterator),
				Position.Y.IsConstant(),
				Config.ScaleXY,
				ScaleZ,
				OffsetZ,
				BicubicSmoothness,
				Heightmap.Data->GetSizeX(),
				Heightmap.Data->GetSizeY(),
				Heightmap.Data->GetHeights().GetData(),
				Iterator.Num(),
				Result.GetData(Iterator));
		}
		break;
		}
	});

	return FVoxelFloatBuffer::Make(Result);
}

FVoxelBox UVoxelHeightmapFunctionLibrary::GetHeightmapBounds(const FVoxelHeightmapRef& Heightmap) const
{
	if (!Heightmap.Data)
	{
		VOXEL_MESSAGE(Error, "{0}: Heightmap is null", this);
		return FVoxelBox::Infinite;
	}
	const FVoxelHeightmapConfig& Config = Heightmap.Config;

	const float ScaleZ = Config.ScaleZ * Config.InternalScaleZ / MAX_uint16;
	const float OffsetZ = Config.ScaleZ * Config.InternalOffsetZ;

	const FVector2D Size = FVector2D(Heightmap.Data->GetSizeX(), Heightmap.Data->GetSizeY()) / 2.f * Config.ScaleXY;
	const float MinHeight = Heightmap.Data->GetMinHeight() * ScaleZ + OffsetZ;
	const float MaxHeight = Heightmap.Data->GetMaxHeight() * ScaleZ + OffsetZ;

	FVoxelBox Bounds;
	Bounds.Min.X = -Size.X;
	Bounds.Min.Y = -Size.Y;
	Bounds.Min.Z = MinHeight;
	Bounds.Max.X = Size.X;
	Bounds.Max.Y = Size.Y;
	Bounds.Max.Z = MaxHeight;
	return Bounds;
}