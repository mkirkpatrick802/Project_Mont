// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "FunctionLibrary/VoxelDistanceFieldFunctionLibrary.h"
#include "VoxelGraphMigration.h"
#include "VoxelPositionQueryParameter.h"
#include "VoxelDistanceFieldFunctionLibraryImpl.ispc.generated.h"

VOXEL_RUN_ON_STARTUP_GAME(VoxelDistanceFunctionLibraryMigration)
{
	REGISTER_VOXEL_FUNCTION_MIGRATION("Make Sphere Surface", UVoxelDistanceFieldFunctionLibrary, CreateSphereDistanceField);
	REGISTER_VOXEL_FUNCTION_MIGRATION("Make Box Surface", UVoxelDistanceFieldFunctionLibrary, CreateBoxDistanceField);

	REGISTER_VOXEL_FUNCTION_PIN_MIGRATION(UVoxelDistanceFieldFunctionLibrary, CreateSphereDistanceField, Surface, ReturnValue)
	REGISTER_VOXEL_FUNCTION_PIN_MIGRATION(UVoxelDistanceFieldFunctionLibrary, CreateBoxDistanceField, Surface, ReturnValue)
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFloatBuffer UVoxelDistanceFieldFunctionLibrary::CreateSphereDistanceField(
	FVoxelBox& Bounds,
	const FVector& Center,
	const float InRadius) const
{
	VOXEL_FUNCTION_COUNTER();

	const float Radius = FMath::Max(InRadius, 0.f);

	Bounds = FVoxelBox(Center - Radius, Center + Radius);

	FindOptionalVoxelQueryParameter_Function(FVoxelPositionQueryParameter, PositionQueryParameter);
	if (!PositionQueryParameter)
	{
		return {};
	}

	const FVoxelVectorBuffer Positions = PositionQueryParameter->GetPositions();

	FVoxelFloatBufferStorage Distance;
	Distance.Allocate(Positions.Num());

	ForeachVoxelBufferChunk_Parallel(Positions.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelDistanceFieldFunctionLibrary_CreateSphereDistanceField(
			Positions.X.GetData(Iterator),
			Positions.X.IsConstant(),
			Positions.Y.GetData(Iterator),
			Positions.Y.IsConstant(),
			Positions.Z.GetData(Iterator),
			Positions.Z.IsConstant(),
			GetISPCValue(FVector3f(Center)),
			Radius,
			Distance.GetData(Iterator),
			Iterator.Num()
		);
	});

	return FVoxelFloatBuffer::Make(Distance);
}

FVoxelFloatBuffer UVoxelDistanceFieldFunctionLibrary::CreateBoxDistanceField(
	FVoxelBox& Bounds,
	const FVector& Center,
	const FVector& InExtent,
	const float InSmoothness) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVector Extent = FVector::Max(InExtent, FVector::ZeroVector);
	const float Smoothness = FMath::Max(InSmoothness, 0.f);

	Bounds = FVoxelBox(Center - Extent, Center + Extent).Extend(Smoothness);

	FindOptionalVoxelQueryParameter_Function(FVoxelPositionQueryParameter, PositionQueryParameter);
	if (!PositionQueryParameter)
	{
		return {};
	}

	const FVoxelVectorBuffer Positions = PositionQueryParameter->GetPositions();

	FVoxelFloatBufferStorage Distance;
	Distance.Allocate(Positions.Num());

	ForeachVoxelBufferChunk_Parallel(Positions.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelDistanceFieldFunctionLibrary_CreateBoxDistanceField(
			Positions.X.GetData(Iterator),
			Positions.X.IsConstant(),
			Positions.Y.GetData(Iterator),
			Positions.Y.IsConstant(),
			Positions.Z.GetData(Iterator),
			Positions.Z.IsConstant(),
			GetISPCValue(FVector3f(Center)),
			GetISPCValue(FVector3f(Extent)),
			Smoothness,
			Distance.GetData(Iterator),
			Iterator.Num()
		);
	});

	return FVoxelFloatBuffer::Make(Distance);
}