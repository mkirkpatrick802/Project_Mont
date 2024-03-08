// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.isph"
#include "Volume/VoxelLandscapeVolumeProviderImpl.isph"

// ReSharper disable CppCStyleCast

export void VoxelLandscapeMeshVolumeProvider_Apply(
	uniform float Distances[],
	const uniform int32 StartX,
	const uniform int32 StartY,
	const uniform int32 StartZ,
	const uniform int32 EndX,
	const uniform int32 EndY,
	const uniform int32 EndZ,
	const uniform int32 StrideX,
	const uniform int32 StrideXY,
	const uniform double3& Translation,
	const uniform double4& Rotation,
	const uniform double3& Scale,
	const uniform EVoxelLandscapeVolumeBlendMode BlendMode,
	const uniform float Smoothness,
	const uniform int3& Size,
	const uniform float DistanceScale,
	const uniform float MeshDistances[])
{
	const uniform float MaxPositionX = Size.x - 1;
	const uniform float MaxPositionY = Size.y - 1;
	const uniform float MaxPositionZ = Size.z - 1;

	const uniform int32 SizeX = Size.x;
	const uniform int32 SizeXY = Size.x * Size.y;

	for (uniform int32 IndexZ = StartZ; IndexZ < EndZ; IndexZ++)
	{
		for (uniform int32 IndexY = StartY; IndexY < EndY; IndexY++)
		{
			FOREACH(IndexX, StartX, EndX)
			{
				const varying double3 IndexPosition = MakeDouble3(IndexX, IndexY, IndexZ);
				const varying double3 LocalPosition = RotateVector(IndexPosition * Scale, Rotation) + Translation;

				const varying float LocalPositionXf = (float)LocalPosition.x;
				const varying float LocalPositionYf = (float)LocalPosition.y;
				const varying float LocalPositionZf = (float)LocalPosition.z;

				coherent_if(
					LocalPositionXf < 0.f ||
					LocalPositionYf < 0.f ||
					LocalPositionZf < 0.f ||
					LocalPositionXf >= MaxPositionX ||
					LocalPositionYf >= MaxPositionY ||
					LocalPositionZf >= MaxPositionZ)
				{
					continue;
				}

				const varying float MinXf = floor(LocalPositionXf);
				const varying float MinYf = floor(LocalPositionYf);
				const varying float MinZf = floor(LocalPositionZf);

				const varying float AlphaX = LocalPositionXf - MinXf;
				const varying float AlphaY = LocalPositionYf - MinYf;
				const varying float AlphaZ = LocalPositionZf - MinZf;

				check(0 <= MinXf && MinXf + 1 < Size.x);
				check(0 <= MinYf && MinYf + 1 < Size.x);
				check(0 <= MinZf && MinZf + 1 < Size.x);

				const varying int32 MinX = (int32)MinXf;
				const varying int32 MinY = (int32)MinYf;
				const varying int32 MinZ = (int32)MinZf;

				IGNORE_PERF_WARNING
				const varying float Distance000 = MeshDistances[(MinX + 0) + SizeX * (MinY + 0) + SizeXY * (MinZ + 0)];
				IGNORE_PERF_WARNING
				const varying float Distance001 = MeshDistances[(MinX + 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 0)];
				IGNORE_PERF_WARNING
				const varying float Distance010 = MeshDistances[(MinX + 0) + SizeX * (MinY + 1) + SizeXY * (MinZ + 0)];
				IGNORE_PERF_WARNING
				const varying float Distance011 = MeshDistances[(MinX + 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 0)];
				IGNORE_PERF_WARNING
				const varying float Distance100 = MeshDistances[(MinX + 0) + SizeX * (MinY + 0) + SizeXY * (MinZ + 1)];
				IGNORE_PERF_WARNING
				const varying float Distance101 = MeshDistances[(MinX + 1) + SizeX * (MinY + 0) + SizeXY * (MinZ + 1)];
				IGNORE_PERF_WARNING
				const varying float Distance110 = MeshDistances[(MinX + 0) + SizeX * (MinY + 1) + SizeXY * (MinZ + 1)];
				IGNORE_PERF_WARNING
				const varying float Distance111 = MeshDistances[(MinX + 1) + SizeX * (MinY + 1) + SizeXY * (MinZ + 1)];

				const varying float NewDistance = DistanceScale * TrilinearInterpolation(
					Distance000,
					Distance001,
					Distance010,
					Distance011,
					Distance100,
					Distance101,
					Distance110,
					Distance111,
					AlphaX,
					AlphaY,
					AlphaZ);

				ApplyVolume(
					Distances,
					IndexX,
					IndexY,
					IndexZ,
					StrideX,
					StrideXY,
					BlendMode,
					Smoothness,
					NewDistance);
			}
		}
	}
}