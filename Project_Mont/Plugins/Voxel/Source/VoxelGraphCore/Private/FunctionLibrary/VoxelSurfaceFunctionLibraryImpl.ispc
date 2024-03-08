// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.isph"
#include "Material/VoxelMaterialBlendingImpl.isph"

// ReSharper disable CppCStyleCast

export void VoxelSurfaceFunctionLibrary_BlendMaterials(
	const uniform uint64 BlendingsA[],
	const uniform bool bConstantBlendingsA,
	const uniform uint64 BlendingsB[],
	const uniform bool bConstantBlendingsB,
	const uniform float BlendAlphas[],
	const uniform bool bConstantBlendAlphas,
	const uniform int32 Num,
	uniform uint64 OutBlending[])
{
	FOREACH(Index, 0, Num)
	{
		varying int32 Materials[6];
		varying float Alphas[6];

		VoxelMaterialBlending_Break(
			bConstantBlendingsA ? BlendingsA[0] : BlendingsA[Index],
			Materials[0],
			Materials[1],
			Materials[2],
			Alphas[0],
			Alphas[1],
			Alphas[2]);

		VoxelMaterialBlending_Break(
			bConstantBlendingsB ? BlendingsB[0] : BlendingsB[Index],
			Materials[3],
			Materials[4],
			Materials[5],
			Alphas[3],
			Alphas[4],
			Alphas[5]);

		const varying float StrengthB = clamp(bConstantBlendAlphas ? BlendAlphas[0] : BlendAlphas[Index], 0.f, 1.f);
		const varying float StrengthA = 1.f - StrengthB;

		Alphas[0] *= StrengthA;
		Alphas[1] *= StrengthA;
		Alphas[2] *= StrengthA;

		Alphas[3] *= StrengthB;
		Alphas[4] *= StrengthB;
		Alphas[5] *= StrengthB;

		UNROLL
		for (uniform int32 IndexA = 0; IndexA < 3; IndexA++)
		{
			UNROLL
			for (uniform int32 IndexB = 3; IndexB < 6; IndexB++)
			{
				if (Materials[IndexA] == Materials[IndexB])
				{
					Alphas[IndexA] += Alphas[IndexB];
					Alphas[IndexB] = 0.f;
				}
			}
		}

		// Optimized version of https://bertdobbelaere.github.io/sorting_networks_extended.html#N6L12D5 for 3 outputs

#define SWAP(A, B) \
		if (Alphas[A] < Alphas[B]) \
		{ \
			const varying float AlphaA = Alphas[A]; \
			Alphas[A] = Alphas[B]; \
			Alphas[B] = AlphaA; \
			\
			const varying float MaterialA = Materials[A]; \
			Materials[A] = Materials[B]; \
			Materials[B] = MaterialA; \
		}

		SWAP(0, 5);
		SWAP(1, 3);
		SWAP(2, 4);

		SWAP(1, 2);
		SWAP(3, 4);

		SWAP(0, 3);
		SWAP(2, 5);

		SWAP(0, 1);
		SWAP(2, 3);

		SWAP(1, 2);

#undef SWAP

		OutBlending[Index] = VoxelMaterialBlending_Make(
			Materials[0],
			Materials[1],
			Materials[2],
			Alphas[0],
			Alphas[1],
			Alphas[2]);
	}
}