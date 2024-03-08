// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Utilities/VoxelBufferConversionUtilities.h"
#include "Point/VoxelPointId.h"
#include "VoxelBufferConversionUtilitiesImpl.ispc.generated.h"

FVoxelFloatBuffer FVoxelBufferConversionUtilities::IntToFloat(const FVoxelInt32Buffer& Buffer)
{
	VOXEL_FUNCTION_COUNTER_NUM(Buffer.Num(), 1024);

	FVoxelFloatBufferStorage Result;
	Result.Allocate(Buffer.Num());

	ForeachVoxelBufferChunk_Parallel(Buffer.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferConversionUtilities_IntToFloat(
			Buffer.GetData(Iterator),
			Result.GetData(Iterator),
			Iterator.Num());
	});

	return FVoxelFloatBuffer::Make(Result);
}

FVoxelDoubleBuffer FVoxelBufferConversionUtilities::IntToDouble(const FVoxelInt32Buffer& Buffer)
{
	VOXEL_FUNCTION_COUNTER_NUM(Buffer.Num(), 1024);

	FVoxelDoubleBufferStorage Result;
	Result.Allocate(Buffer.Num());

	ForeachVoxelBufferChunk_Parallel(Buffer.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferConversionUtilities_IntToDouble(
			Buffer.GetData(Iterator),
			Result.GetData(Iterator),
			Iterator.Num());
	});

	return FVoxelDoubleBuffer::Make(Result);
}

FVoxelDoubleBuffer FVoxelBufferConversionUtilities::FloatToDouble(const FVoxelFloatBuffer& Float)
{
	VOXEL_FUNCTION_COUNTER_NUM(Float.Num(), 1024);

	FVoxelDoubleBufferStorage Result;
	Result.Allocate(Float.Num());

	ForeachVoxelBufferChunk_Parallel(Float.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferConversionUtilities_FloatToDouble(
			Float.GetData(Iterator),
			Result.GetData(Iterator),
			Iterator.Num());
	});

	return FVoxelDoubleBuffer::Make(Result);
}

FVoxelFloatBuffer FVoxelBufferConversionUtilities::DoubleToFloat(const FVoxelDoubleBuffer& Double)
{
	VOXEL_FUNCTION_COUNTER_NUM(Double.Num(), 1024);

	FVoxelFloatBufferStorage Result;
	Result.Allocate(Double.Num());

	ForeachVoxelBufferChunk_Parallel(Double.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferConversionUtilities_DoubleToFloat(
			Double.GetData(Iterator),
			Result.GetData(Iterator),
			Iterator.Num());
	});

	return FVoxelFloatBuffer::Make(Result);
}

FVoxelSeedBuffer FVoxelBufferConversionUtilities::PointIdToSeed(const FVoxelPointIdBuffer& Buffer)
{
	VOXEL_FUNCTION_COUNTER_NUM(Buffer.Num(), 1024);

	FVoxelSeedBufferStorage Result;
	Result.Allocate(Buffer.Num());

	ForeachVoxelBufferChunk_Parallel(Buffer.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferConversionUtilities_PointIdToSeed(
			ReinterpretCastPtr<uint64>(Buffer.GetData(Iterator)),
			ReinterpretCastPtr<uint32>(Result.GetData(Iterator)),
			Iterator.Num());
	});

	return FVoxelSeedBuffer::Make(Result);
}