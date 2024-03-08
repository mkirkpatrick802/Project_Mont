// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Utilities/VoxelBufferMathUtilities.h"
#include "VoxelBufferMathUtilitiesImpl.ispc.generated.h"

FVoxelFloatBuffer FVoxelBufferMathUtilities::Min8(
	const FVoxelFloatBuffer& Buffer0,
	const FVoxelFloatBuffer& Buffer1,
	const FVoxelFloatBuffer& Buffer2,
	const FVoxelFloatBuffer& Buffer3,
	const FVoxelFloatBuffer& Buffer4,
	const FVoxelFloatBuffer& Buffer5,
	const FVoxelFloatBuffer& Buffer6,
	const FVoxelFloatBuffer& Buffer7)
{
	const FVoxelBufferAccessor BufferAccessor(
		Buffer0,
		Buffer1,
		Buffer2,
		Buffer3,
		Buffer4,
		Buffer5,
		Buffer6,
		Buffer7);

	if (!ensure(BufferAccessor.IsValid()))
	{
		return {};
	}

	VOXEL_FUNCTION_COUNTER_NUM(BufferAccessor.Num(), 1024);

	FVoxelFloatBufferStorage Result;
	Result.Allocate(BufferAccessor.Num());

	ForeachVoxelBufferChunk_Parallel(BufferAccessor.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferMathUtilities_Min8(
			Buffer0.GetData(Iterator), Buffer0.IsConstant(),
			Buffer1.GetData(Iterator), Buffer1.IsConstant(),
			Buffer2.GetData(Iterator), Buffer2.IsConstant(),
			Buffer3.GetData(Iterator), Buffer3.IsConstant(),
			Buffer4.GetData(Iterator), Buffer4.IsConstant(),
			Buffer5.GetData(Iterator), Buffer5.IsConstant(),
			Buffer6.GetData(Iterator), Buffer6.IsConstant(),
			Buffer7.GetData(Iterator), Buffer7.IsConstant(),
			Result.GetData(Iterator),
			Iterator.Num());
	});

	return FVoxelFloatBuffer::Make(Result);
}

FVoxelFloatBuffer FVoxelBufferMathUtilities::Max8(
	const FVoxelFloatBuffer& Buffer0,
	const FVoxelFloatBuffer& Buffer1,
	const FVoxelFloatBuffer& Buffer2,
	const FVoxelFloatBuffer& Buffer3,
	const FVoxelFloatBuffer& Buffer4,
	const FVoxelFloatBuffer& Buffer5,
	const FVoxelFloatBuffer& Buffer6,
	const FVoxelFloatBuffer& Buffer7)
{
	const FVoxelBufferAccessor BufferAccessor(
		Buffer0,
		Buffer1,
		Buffer2,
		Buffer3,
		Buffer4,
		Buffer5,
		Buffer6,
		Buffer7);

	if (!ensure(BufferAccessor.IsValid()))
	{
		return {};
	}

	VOXEL_FUNCTION_COUNTER_NUM(BufferAccessor.Num(), 1024);

	FVoxelFloatBufferStorage Result;
	Result.Allocate(BufferAccessor.Num());

	ForeachVoxelBufferChunk_Parallel(BufferAccessor.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferMathUtilities_Max8(
			Buffer0.GetData(Iterator), Buffer0.IsConstant(),
			Buffer1.GetData(Iterator), Buffer1.IsConstant(),
			Buffer2.GetData(Iterator), Buffer2.IsConstant(),
			Buffer3.GetData(Iterator), Buffer3.IsConstant(),
			Buffer4.GetData(Iterator), Buffer4.IsConstant(),
			Buffer5.GetData(Iterator), Buffer5.IsConstant(),
			Buffer6.GetData(Iterator), Buffer6.IsConstant(),
			Buffer7.GetData(Iterator), Buffer7.IsConstant(),
			Result.GetData(Iterator),
			Iterator.Num());
	});

	return FVoxelFloatBuffer::Make(Result);
}

FVoxelVectorBuffer FVoxelBufferMathUtilities::Min8(
	const FVoxelVectorBuffer& Buffer0,
	const FVoxelVectorBuffer& Buffer1,
	const FVoxelVectorBuffer& Buffer2,
	const FVoxelVectorBuffer& Buffer3,
	const FVoxelVectorBuffer& Buffer4,
	const FVoxelVectorBuffer& Buffer5,
	const FVoxelVectorBuffer& Buffer6,
	const FVoxelVectorBuffer& Buffer7)
{
	FVoxelVectorBuffer Result;
	Result.X = Min8(Buffer0.X, Buffer1.X, Buffer2.X, Buffer3.X, Buffer4.X, Buffer5.X, Buffer6.X, Buffer7.X);
	Result.Y = Min8(Buffer0.Y, Buffer1.Y, Buffer2.Y, Buffer3.Y, Buffer4.Y, Buffer5.Y, Buffer6.Y, Buffer7.Y);
	Result.Z = Min8(Buffer0.Z, Buffer1.Z, Buffer2.Z, Buffer3.Z, Buffer4.Z, Buffer5.Z, Buffer6.Z, Buffer7.Z);
	return Result;
}

FVoxelVectorBuffer FVoxelBufferMathUtilities::Max8(
	const FVoxelVectorBuffer& Buffer0,
	const FVoxelVectorBuffer& Buffer1,
	const FVoxelVectorBuffer& Buffer2,
	const FVoxelVectorBuffer& Buffer3,
	const FVoxelVectorBuffer& Buffer4,
	const FVoxelVectorBuffer& Buffer5,
	const FVoxelVectorBuffer& Buffer6,
	const FVoxelVectorBuffer& Buffer7)
{
	FVoxelVectorBuffer Result;
	Result.X = Max8(Buffer0.X, Buffer1.X, Buffer2.X, Buffer3.X, Buffer4.X, Buffer5.X, Buffer6.X, Buffer7.X);
	Result.Y = Max8(Buffer0.Y, Buffer1.Y, Buffer2.Y, Buffer3.Y, Buffer4.Y, Buffer5.Y, Buffer6.Y, Buffer7.Y);
	Result.Z = Max8(Buffer0.Z, Buffer1.Z, Buffer2.Z, Buffer3.Z, Buffer4.Z, Buffer5.Z, Buffer6.Z, Buffer7.Z);
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFloatBuffer FVoxelBufferMathUtilities::Add(const FVoxelFloatBuffer& A, const FVoxelFloatBuffer& B)
{
	const FVoxelBufferAccessor BufferAccessor(A, B);
	if (!ensure(BufferAccessor.IsValid()))
	{
		return {};
	}

	VOXEL_FUNCTION_COUNTER_NUM(BufferAccessor.Num(), 1024);

	FVoxelFloatBufferStorage Result;
	Result.Allocate(BufferAccessor.Num());

	ForeachVoxelBufferChunk_Parallel(BufferAccessor.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferMathUtilities_Add(
			A.GetData(Iterator),
			A.IsConstant(),
			B.GetData(Iterator),
			B.IsConstant(),
			Result.GetData(Iterator),
			Iterator.Num());
	});

	return FVoxelFloatBuffer::Make(Result);
}

FVoxelFloatBuffer FVoxelBufferMathUtilities::Multiply(const FVoxelFloatBuffer& A, const FVoxelFloatBuffer& B)
{
	const FVoxelBufferAccessor BufferAccessor(A, B);
	if (!ensure(BufferAccessor.IsValid()))
	{
		return {};
	}

	VOXEL_FUNCTION_COUNTER_NUM(BufferAccessor.Num(), 1024);

	FVoxelFloatBufferStorage Result;
	Result.Allocate(BufferAccessor.Num());

	ForeachVoxelBufferChunk_Parallel(BufferAccessor.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferMathUtilities_Multiply(
			A.GetData(Iterator),
			A.IsConstant(),
			B.GetData(Iterator),
			B.IsConstant(),
			Result.GetData(Iterator),
			Iterator.Num());
	});

	return FVoxelFloatBuffer::Make(Result);
}

FVoxelBoolBuffer FVoxelBufferMathUtilities::Less(const FVoxelFloatBuffer& A, const FVoxelFloatBuffer& B)
{
	const FVoxelBufferAccessor BufferAccessor(A, B);
	if (!ensure(BufferAccessor.IsValid()))
	{
		return {};
	}

	VOXEL_FUNCTION_COUNTER_NUM(BufferAccessor.Num(), 1024);

	FVoxelBoolBufferStorage Result;
	Result.Allocate(BufferAccessor.Num());

	ForeachVoxelBufferChunk_Parallel(BufferAccessor.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferMathUtilities_Less(
			A.GetData(Iterator),
			A.IsConstant(),
			B.GetData(Iterator),
			B.IsConstant(),
			Result.GetData(Iterator),
			Iterator.Num());
	});

	return FVoxelBoolBuffer::Make(Result);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelVectorBuffer FVoxelBufferMathUtilities::Add(const FVoxelVectorBuffer& A, const FVoxelVectorBuffer& B)
{
	FVoxelVectorBuffer Result;
	Result.X = Add(A.X, B.X);
	Result.Y = Add(A.Y, B.Y);
	Result.Z = Add(A.Z, B.Z);
	return Result;
}

FVoxelVectorBuffer FVoxelBufferMathUtilities::Multiply(const FVoxelVectorBuffer& A, const FVoxelVectorBuffer& B)
{
	FVoxelVectorBuffer Result;
	Result.X = Multiply(A.X, B.X);
	Result.Y = Multiply(A.Y, B.Y);
	Result.Z = Multiply(A.Z, B.Z);
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFloatBuffer FVoxelBufferMathUtilities::Lerp(const FVoxelFloatBuffer& A, const FVoxelFloatBuffer& B, const FVoxelFloatBuffer& Alpha)
{
	const FVoxelBufferAccessor BufferAccessor(A, B, Alpha);
	if (!ensure(BufferAccessor.IsValid()))
	{
		return {};
	}

	VOXEL_FUNCTION_COUNTER_NUM(BufferAccessor.Num(), 1024);

	FVoxelFloatBufferStorage Result;
	Result.Allocate(BufferAccessor.Num());

	ForeachVoxelBufferChunk_Parallel(BufferAccessor.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferMathUtilities_Alpha(
			A.GetData(Iterator),
			A.IsConstant(),
			B.GetData(Iterator),
			B.IsConstant(),
			Alpha.GetData(Iterator),
			Alpha.IsConstant(),
			Result.GetData(Iterator),
			Iterator.Num());
	});

	return FVoxelFloatBuffer::Make(Result);
}

FVoxelVector2DBuffer FVoxelBufferMathUtilities::Lerp(const FVoxelVector2DBuffer& A, const FVoxelVector2DBuffer& B, const FVoxelFloatBuffer& Alpha)
{
	FVoxelVector2DBuffer Result;
	Result.X = Lerp(A.X, B.X, Alpha);
	Result.Y = Lerp(A.Y, B.Y, Alpha);
	return Result;
}

FVoxelVectorBuffer FVoxelBufferMathUtilities::Lerp(const FVoxelVectorBuffer& A, const FVoxelVectorBuffer& B, const FVoxelFloatBuffer& Alpha)
{
	FVoxelVectorBuffer Result;
	Result.X = Lerp(A.X, B.X, Alpha);
	Result.Y = Lerp(A.Y, B.Y, Alpha);
	Result.Z = Lerp(A.Z, B.Z, Alpha);
	return Result;
}

FVoxelLinearColorBuffer FVoxelBufferMathUtilities::Lerp(const FVoxelLinearColorBuffer& A, const FVoxelLinearColorBuffer& B, const FVoxelFloatBuffer& Alpha)
{
	FVoxelLinearColorBuffer Result;
	Result.R = Lerp(A.R, B.R, Alpha);
	Result.G = Lerp(A.G, B.G, Alpha);
	Result.B = Lerp(A.B, B.B, Alpha);
	Result.A = Lerp(A.A, B.A, Alpha);
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelQuaternionBuffer FVoxelBufferMathUtilities::Combine(const FVoxelQuaternionBuffer& A, const FVoxelQuaternionBuffer& B)
{
	const FVoxelBufferAccessor BufferAccessor(A, B);
	if (!ensure(BufferAccessor.IsValid()))
	{
		return {};
	}

	VOXEL_FUNCTION_COUNTER_NUM(BufferAccessor.Num(), 1024);

	FVoxelFloatBufferStorage X;
	FVoxelFloatBufferStorage Y;
	FVoxelFloatBufferStorage Z;
	FVoxelFloatBufferStorage W;
	X.Allocate(BufferAccessor.Num());
	Y.Allocate(BufferAccessor.Num());
	Z.Allocate(BufferAccessor.Num());
	W.Allocate(BufferAccessor.Num());

	ForeachVoxelBufferChunk_Parallel(BufferAccessor.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferMathUtilities_Combine(
			A.X.GetData(Iterator),
			A.X.IsConstant(),
			A.Y.GetData(Iterator),
			A.Y.IsConstant(),
			A.Z.GetData(Iterator),
			A.Z.IsConstant(),
			A.W.GetData(Iterator),
			A.W.IsConstant(),
			B.X.GetData(Iterator),
			B.X.IsConstant(),
			B.Y.GetData(Iterator),
			B.Y.IsConstant(),
			B.Z.GetData(Iterator),
			B.Z.IsConstant(),
			B.W.GetData(Iterator),
			B.W.IsConstant(),
			X.GetData(Iterator),
			Y.GetData(Iterator),
			Z.GetData(Iterator),
			W.GetData(Iterator),
			Iterator.Num());
	});

	return FVoxelQuaternionBuffer::Make(X, Y, Z, W);
}

FVoxelVectorBuffer FVoxelBufferMathUtilities::MakeEulerFromQuaternion(const FVoxelQuaternionBuffer& Quaternion)
{
	VOXEL_FUNCTION_COUNTER_NUM(Quaternion.Num(), 1024);

	FVoxelFloatBufferStorage X;
	FVoxelFloatBufferStorage Y;
	FVoxelFloatBufferStorage Z;
	X.Allocate(Quaternion.Num());
	Y.Allocate(Quaternion.Num());
	Z.Allocate(Quaternion.Num());

	ForeachVoxelBufferChunk_Parallel(Quaternion.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferMathUtilities_MakeEulerFromQuaternion(
			Quaternion.X.GetData(Iterator),
			Quaternion.X.IsConstant(),
			Quaternion.Y.GetData(Iterator),
			Quaternion.Y.IsConstant(),
			Quaternion.Z.GetData(Iterator),
			Quaternion.Z.IsConstant(),
			Quaternion.W.GetData(Iterator),
			Quaternion.W.IsConstant(),
			X.GetData(Iterator),
			Y.GetData(Iterator),
			Z.GetData(Iterator),
			Iterator.Num());
	});

	return FVoxelVectorBuffer::Make(X, Y, Z);
}