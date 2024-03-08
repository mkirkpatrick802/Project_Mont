// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Height/VoxelLandscapeHeightProvider.h"

enum class EVoxelLandscapeHeightBlendMode : uint8;
enum class EVoxelLandscapeHeightmapInterpolation : uint8;

struct FVoxelLandscapeHeightUtilities
{
public:
	template<typename T>
	struct TArgs
	{
		TVoxelArrayView<float> InOutHeights;
		FIntRect Span;
		int32 StrideX;
		FTransform2d IndexToHeightmap;
		float ScaleZ;
		float OffsetZ;
		EVoxelLandscapeHeightBlendMode BlendMode;
		float Smoothness;
		float MinHeight;
		float MaxHeight;
		float LocalScaleZ;
		float LocalOffsetZ;
		float BicubicSmoothness;
		EVoxelLandscapeHeightmapInterpolation Interpolation;
		int32 SizeX;
		int32 SizeY;
		TConstVoxelArrayView64<T> Heightmap;

		TArgs(
			const FVoxelLandscapeHeightQuery& Query,
			const float ScaleXY,
			const float LocalScaleZ,
			const float LocalOffsetZ,
			const float BicubicSmoothness,
			const EVoxelLandscapeHeightmapInterpolation Interpolation,
			const int32 SizeX,
			const int32 SizeY,
			const TConstVoxelArrayView64<T>& Heightmap)
			: InOutHeights(Query.Heights)
			, Span(Query.Span)
			, StrideX(Query.StrideX)
			, ScaleZ(Query.ScaleZ)
			, OffsetZ(Query.OffsetZ)
			, BlendMode(Query.BlendMode)
			, Smoothness(Query.Smoothness)
			, MinHeight(Query.MinHeight)
			, MaxHeight(Query.MaxHeight)
			, LocalScaleZ(LocalScaleZ)
			, LocalOffsetZ(LocalOffsetZ)
			, BicubicSmoothness(BicubicSmoothness)
			, Interpolation(Interpolation)
			, SizeX(SizeX)
			, SizeY(SizeY)
			, Heightmap(Heightmap)
		{
			IndexToHeightmap =
				Query.IndexToBrush *
				FTransform2d(FScale2d(ScaleXY).Inverse()) *
				FTransform2d(FVector2d(
					SizeX / 2.,
					SizeY / 2.));
		}
	};

	static void ApplyHeightmap(TArgs<uint8> Args);
	static void ApplyHeightmap(TArgs<uint16> Args);
	static void ApplyHeightmap(TArgs<float> Args);

public:
	static void ComputeNormals(
		TConstVoxelArrayView<float> Heights,
		int32 HeightsSize,
		TVoxelArrayView<FVoxelOctahedron> OutNormals,
		int32 NormalsSize,
		float StepZ);
};