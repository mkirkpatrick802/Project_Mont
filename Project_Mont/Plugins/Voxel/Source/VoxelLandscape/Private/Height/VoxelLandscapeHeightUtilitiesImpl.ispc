// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.isph"

// ReSharper disable CppCStyleCast

#define Type uint8
#include "VoxelLandscapeHeightUtilitiesImpl.isph"
#undef Type

#define Type uint16
#include "VoxelLandscapeHeightUtilitiesImpl.isph"
#undef Type

#define Type float
#include "VoxelLandscapeHeightUtilitiesImpl.isph"
#undef Type

export void VoxelLandscapeHeightUtilities_ComputeNormals(
	const uniform float Heights[],
	const uniform int32 HeightsSize,
	uniform uint16 OutNormals[],
	const uniform int32 NormalsSize,
	const uniform float StepZ)
{
	check(HeightsSize == NormalsSize + 2);

	for (uniform int32 Y = 0; Y < NormalsSize; Y++)
	{
		const uniform int32 IndexY = Y + 1;

		FOREACH(X, 0, NormalsSize)
		{
			const varying int32 IndexX = X + 1;

			const varying float MinX = Heights[IndexX - 1 + HeightsSize * IndexY];
			const varying float MaxX = Heights[IndexX + 1 + HeightsSize * IndexY];

			const varying float MinY = Heights[IndexX + HeightsSize * (IndexY - 1)];
			const varying float MaxY = Heights[IndexX + HeightsSize * (IndexY + 1)];

			float3 Normal;
			// SDFMinX = Z - HeightMinX
			// SDFMaxX = Z - HeightMaxX
			// SDFMaxX - SDFMinX = HeightMinX - HeightMaxX
			Normal.x = MinX - MaxX;
			Normal.y = MinY - MaxY;
			// SDFMinZ = (Z - 1) - Height
			// SDFMaxZ = (Z + 1) - Height
			// SDFMaxZ - SDFMinZ = 2
			Normal.z = 2.f * StepZ;

			coherent_if(
				intbits(MinX) == 0xFFFFFFFF ||
				intbits(MaxX) == 0xFFFFFFFF ||
				intbits(MinY) == 0xFFFFFFFF ||
				intbits(MaxY) == 0xFFFFFFFF)
			{
				Normal.x = 0;
                Normal.y = 0;
                Normal.z = 1;
			}
			else
			{
				Normal = Normal / length(Normal);
			}

            const varying float2 Octahedron = UnitVectorToOctahedron(Normal);
			const varying uint32 Packed32 = (uint32)FloatToUINT8(Octahedron.x) | ((uint32)FloatToUINT8(Octahedron.y) << 8);
			const varying uint16 Packed16 = Packed32;

			OutNormals[X + NormalsSize * Y] = Packed16;
		}
	}
}