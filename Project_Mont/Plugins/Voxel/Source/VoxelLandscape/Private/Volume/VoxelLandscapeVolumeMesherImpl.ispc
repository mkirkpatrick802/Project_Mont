// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.isph"

// ReSharper disable CppCStyleCast

export void VoxelLandscapeVolumeMesher_HeightToDistance(
	const uniform int32 Size,
	const uniform float BaseZ,
	const uniform float Step,
	const uniform float Heights[],
	uniform float Distances[])
{
	const uniform int32 SizeXY = Size * Size;

	FOREACH(Index, 0, SizeXY)
	{
		const varying float Height = Heights[Index];
		const varying bool Mask = intbits(Height) != 0xFFFFFFFF;

		if (none(Mask))
		{
			for (uniform int32 Z = 0; Z < Size; Z++)
			{
				Distances[Index + Z * SizeXY] = floatbits(0xFFFFFFFF);
			}
			continue;
		}

		const varying float BaseHeight = Height - BaseZ;

		if (all(Mask))
		{
			for (uniform int32 Z = 0; Z < Size; Z++)
			{
				Distances[Index + Z * SizeXY] = Z * Step - BaseHeight;
			}
			continue;
		}

		for (uniform int32 Z = 0; Z < Size; Z++)
		{
			Distances[Index + Z * SizeXY] = Mask
				? Z * Step - BaseHeight
				: floatbits(0xFFFFFFFF);
		}
	}
}