// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.isph"

export void VoxelNode_InvertSurface(
	const uniform float Distance[],
	uniform float OutDistance[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		OutDistance[Index] = -Distance[Index];
	}
}

export void VoxelNode_MaskSurface(
	const uniform float Distance[],
	const uniform bool bConstDistance,
	const uniform float MaskDistance[],
	const uniform bool bConstMaskDistance,
	const uniform float Smoothness,
	uniform float OutDistance[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		OutDistance[Index] = SmoothMax(
			bConstDistance ? Distance[0] : Distance[Index],
			bConstMaskDistance ? MaskDistance[0] : MaskDistance[Index],
			Smoothness);
	}
}

export void VoxelNode_InflateSurface(
	const uniform float Distance[],
	const uniform bool bConstDistance,
	const uniform float Amount,
	uniform float OutDistance[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		OutDistance[Index] = (bConstDistance ? Distance[0] : Distance[Index]) - Amount;
	}
}

export void VoxelNode_DisplaceSurface(
	const uniform float DisplacementsX[],
	const uniform bool bConstDisplacementsX,
	const uniform float DisplacementsY[],
	const uniform bool bConstDisplacementsY,
	const uniform float DisplacementsZ[],
	const uniform bool bConstDisplacementsZ,
	const uniform float PositionsX[],
	const uniform bool bConstPositionsX,
	const uniform float PositionsY[],
	const uniform bool bConstPositionsY,
	const uniform float PositionsZ[],
	const uniform bool bConstPositionsZ,
	const uniform float MaxDisplacement,
	uniform float OutPositionX[],
	uniform float OutPositionY[],
	uniform float OutPositionZ[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const varying float DisplacementX = bConstDisplacementsX ? DisplacementsX[0] : DisplacementsX[Index];
		const varying float DisplacementY = bConstDisplacementsY ? DisplacementsY[0] : DisplacementsY[Index];
		const varying float DisplacementZ = bConstDisplacementsZ ? DisplacementsZ[0] : DisplacementsZ[Index];

		const varying float PositionX = bConstPositionsX ? PositionsX[0] : PositionsX[Index];
		const varying float PositionY = bConstPositionsY ? PositionsY[0] : PositionsY[Index];
		const varying float PositionZ = bConstPositionsZ ? PositionsZ[0] : PositionsZ[Index];

		OutPositionX[Index] = PositionX - clamp(DisplacementX, -MaxDisplacement, MaxDisplacement);
		OutPositionY[Index] = PositionY - clamp(DisplacementY, -MaxDisplacement, MaxDisplacement);
		OutPositionZ[Index] = PositionZ - clamp(DisplacementZ, -MaxDisplacement, MaxDisplacement);
	}
}