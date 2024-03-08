// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.isph"

export void VoxelDistanceFieldFunctionLibrary_CreateSphereDistanceField(
	const uniform float PositionX[],
	const uniform bool bConstantPositionX,
	const uniform float PositionY[],
	const uniform bool bConstantPositionY,
	const uniform float PositionZ[],
	const uniform bool bConstantPositionZ,
	const uniform float3& Center,
	const uniform float Radius,
	uniform float OutDistance[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const varying float3 Position = MakeFloat3(
			bConstantPositionX ? PositionX[0] : PositionX[Index],
			bConstantPositionY ? PositionY[0] : PositionY[Index],
			bConstantPositionZ ? PositionZ[0] : PositionZ[Index]);

		OutDistance[Index] = length(Position - Center) - Radius;
	}
}

export void VoxelDistanceFieldFunctionLibrary_CreateBoxDistanceField(
	const uniform float PositionX[],
	const uniform bool bConstantPositionX,
	const uniform float PositionY[],
	const uniform bool bConstantPositionY,
	const uniform float PositionZ[],
	const uniform bool bConstantPositionZ,
	const uniform float3& Center,
	const uniform float3& Extent,
	const uniform float Smoothness,
	uniform float OutDistance[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const varying float3 Position = MakeFloat3(
			bConstantPositionX ? PositionX[0] : PositionX[Index],
			bConstantPositionY ? PositionY[0] : PositionY[Index],
			bConstantPositionZ ? PositionZ[0] : PositionZ[Index]);

		const float3 Q = abs(Position - Center) - Extent;
		OutDistance[Index] = length(max(Q, 0.f)) + min(max(Q.x, max(Q.y, Q.z)), 0.f) - Smoothness;
	}
}