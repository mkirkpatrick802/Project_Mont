// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Utilities/VoxelBufferTransformUtilities.h"
#include "VoxelBufferTransformUtilitiesImpl.ispc.generated.h"

FVoxelVectorBuffer FVoxelBufferTransformUtilities::ApplyTransform(const FVoxelVectorBuffer& Buffer, const FTransform& Transform)
{
	if (Transform.Equals(FTransform::Identity))
	{
		return Buffer;
	}

	VOXEL_FUNCTION_COUNTER_NUM(Buffer.Num(), 1024);

	FVoxelFloatBufferStorage ResultX; ResultX.Allocate(Buffer.Num());
	FVoxelFloatBufferStorage ResultY; ResultY.Allocate(Buffer.Num());
	FVoxelFloatBufferStorage ResultZ; ResultZ.Allocate(Buffer.Num());

	ForeachVoxelBufferChunk_Parallel(Buffer.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferTransformUtilities_ApplyTransform(
			Buffer.X.GetData(Iterator), Buffer.X.IsConstant(),
			Buffer.Y.GetData(Iterator), Buffer.Y.IsConstant(),
			Buffer.Z.GetData(Iterator), Buffer.Z.IsConstant(),
			Iterator.Num(),
			GetISPCValue(FVector3f(Transform.GetTranslation())),
			GetISPCValue(FVector4f(
				Transform.GetRotation().X,
				Transform.GetRotation().Y,
				Transform.GetRotation().Z,
				Transform.GetRotation().W)),
			GetISPCValue(FVector3f(Transform.GetScale3D())),
			ResultX.GetData(Iterator),
			ResultY.GetData(Iterator),
			ResultZ.GetData(Iterator));
	});

	FVoxelVectorBuffer Result;
	Result.X = FVoxelFloatBuffer::Make(ResultX);
	Result.Y = FVoxelFloatBuffer::Make(ResultY);
	Result.Z = FVoxelFloatBuffer::Make(ResultZ);
	return Result;
}

FVoxelVectorBuffer FVoxelBufferTransformUtilities::ApplyInverseTransform(const FVoxelVectorBuffer& Buffer, const FTransform& Transform)
{
	if (Transform.Equals(FTransform::Identity))
	{
		return Buffer;
	}

	VOXEL_FUNCTION_COUNTER_NUM(Buffer.Num(), 1024);

	FVoxelFloatBufferStorage ResultX; ResultX.Allocate(Buffer.Num());
	FVoxelFloatBufferStorage ResultY; ResultY.Allocate(Buffer.Num());
	FVoxelFloatBufferStorage ResultZ; ResultZ.Allocate(Buffer.Num());

	ForeachVoxelBufferChunk_Parallel(Buffer.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferTransformUtilities_ApplyInverseTransform(
			Buffer.X.GetData(Iterator), Buffer.X.IsConstant(),
			Buffer.Y.GetData(Iterator), Buffer.Y.IsConstant(),
			Buffer.Z.GetData(Iterator), Buffer.Z.IsConstant(),
			Iterator.Num(),
			GetISPCValue(FVector3f(Transform.GetTranslation())),
			GetISPCValue(FVector4f(
				Transform.GetRotation().X,
				Transform.GetRotation().Y,
				Transform.GetRotation().Z,
				Transform.GetRotation().W)),
			GetISPCValue(FVector3f(Transform.GetScale3D())),
			ResultX.GetData(Iterator),
			ResultY.GetData(Iterator),
			ResultZ.GetData(Iterator));
	});

	FVoxelVectorBuffer Result;
	Result.X = FVoxelFloatBuffer::Make(ResultX);
	Result.Y = FVoxelFloatBuffer::Make(ResultY);
	Result.Z = FVoxelFloatBuffer::Make(ResultZ);
	return Result;
}

FVoxelVectorBuffer FVoxelBufferTransformUtilities::ApplyTransform(
	const FVoxelVectorBuffer& Buffer,
	const FVoxelVectorBuffer& Translation,
	const FVoxelQuaternionBuffer& Rotation,
	const FVoxelVectorBuffer& Scale)
{
	const FVoxelBufferAccessor BufferAccessor(Buffer, Translation, Rotation, Scale);
	if (!ensure(BufferAccessor.IsValid()))
	{
		return {};
	}

	VOXEL_FUNCTION_COUNTER_NUM(BufferAccessor.Num(), 1024);

	FVoxelFloatBufferStorage ResultX; ResultX.Allocate(BufferAccessor.Num());
	FVoxelFloatBufferStorage ResultY; ResultY.Allocate(BufferAccessor.Num());
	FVoxelFloatBufferStorage ResultZ; ResultZ.Allocate(BufferAccessor.Num());

	ForeachVoxelBufferChunk_Parallel(BufferAccessor.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferTransformUtilities_ApplyTransform_Bulk(
			Buffer.X.GetData(Iterator), Buffer.X.IsConstant(),
			Buffer.Y.GetData(Iterator), Buffer.Y.IsConstant(),
			Buffer.Z.GetData(Iterator), Buffer.Z.IsConstant(),
			Translation.X.GetData(Iterator), Translation.X.IsConstant(),
			Translation.Y.GetData(Iterator), Translation.Y.IsConstant(),
			Translation.Z.GetData(Iterator), Translation.Z.IsConstant(),
			Rotation.X.GetData(Iterator), Rotation.X.IsConstant(),
			Rotation.Y.GetData(Iterator), Rotation.Y.IsConstant(),
			Rotation.Z.GetData(Iterator), Rotation.Z.IsConstant(),
			Rotation.W.GetData(Iterator), Rotation.W.IsConstant(),
			Scale.X.GetData(Iterator), Scale.X.IsConstant(),
			Scale.Y.GetData(Iterator), Scale.Y.IsConstant(),
			Scale.Z.GetData(Iterator), Scale.Z.IsConstant(),
			Iterator.Num(),
			ResultX.GetData(Iterator),
			ResultY.GetData(Iterator),
			ResultZ.GetData(Iterator));
	});

	return FVoxelVectorBuffer::Make(ResultX, ResultY, ResultZ);
}

FVoxelVectorBuffer FVoxelBufferTransformUtilities::ApplyTransform(const FVoxelVectorBuffer& Buffer, const FMatrix& Matrix)
{
	const FTransform Transform(Matrix);
	if (Transform.ToMatrixWithScale().Equals(Matrix))
	{
		return ApplyTransform(Buffer, Transform);
	}

	ensure(!Matrix.GetScaleVector().IsUniform());
	VOXEL_SCOPE_COUNTER_NUM("Non-Uniform Scale", Buffer.Num(), 1024);

	const FVector Translation = Matrix.GetOrigin();
	const FMatrix BaseMatrix = Matrix.RemoveTranslation();
	ensure(Matrix.Equals(BaseMatrix * FTranslationMatrix(Translation)));

	ensure(FMath::IsNearlyEqual(BaseMatrix.M[0][3], 0));
	ensure(FMath::IsNearlyEqual(BaseMatrix.M[1][3], 0));
	ensure(FMath::IsNearlyEqual(BaseMatrix.M[2][3], 0));
	ensure(FMath::IsNearlyEqual(BaseMatrix.M[3][0], 0));
	ensure(FMath::IsNearlyEqual(BaseMatrix.M[3][1], 0));
	ensure(FMath::IsNearlyEqual(BaseMatrix.M[3][2], 0));
	ensure(FMath::IsNearlyEqual(BaseMatrix.M[3][3], 1));

	FVoxelFloatBufferStorage ResultX; ResultX.Allocate(Buffer.Num());
	FVoxelFloatBufferStorage ResultY; ResultY.Allocate(Buffer.Num());
	FVoxelFloatBufferStorage ResultZ; ResultZ.Allocate(Buffer.Num());

	ForeachVoxelBufferChunk_Parallel(Buffer.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferTransformUtilities_ApplyTransform_Matrix(
			Buffer.X.GetData(Iterator), Buffer.X.IsConstant(),
			Buffer.Y.GetData(Iterator), Buffer.Y.IsConstant(),
			Buffer.Z.GetData(Iterator), Buffer.Z.IsConstant(),
			Iterator.Num(),
			GetISPCValue(FVector3f(Translation)),
			GetISPCValue(FVector3f(BaseMatrix.M[0][0], BaseMatrix.M[0][1], BaseMatrix.M[0][2])),
			GetISPCValue(FVector3f(BaseMatrix.M[1][0], BaseMatrix.M[1][1], BaseMatrix.M[1][2])),
			GetISPCValue(FVector3f(BaseMatrix.M[2][0], BaseMatrix.M[2][1], BaseMatrix.M[2][2])),
			ResultX.GetData(Iterator),
			ResultY.GetData(Iterator),
			ResultZ.GetData(Iterator));
	});

	FVoxelVectorBuffer Result;
	Result.X = FVoxelFloatBuffer::Make(ResultX);
	Result.Y = FVoxelFloatBuffer::Make(ResultY);
	Result.Z = FVoxelFloatBuffer::Make(ResultZ);
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFloatBuffer FVoxelBufferTransformUtilities::TransformDistance(const FVoxelFloatBuffer& Distance, const FMatrix& Transform)
{
	const float Scale = Transform.GetScaleVector().GetAbsMax();
	if (Scale == 1.f)
	{
		return Distance;
	}

	VOXEL_FUNCTION_COUNTER_NUM(Distance.Num(), 1024);

	FVoxelFloatBufferStorage Result;
	Result.Allocate(Distance.Num());

	ForeachVoxelBufferChunk_Parallel(Distance.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelBufferTransformUtilities_TransformDistance(
			Distance.GetData(Iterator),
			Result.GetData(Iterator),
			Scale,
			Iterator.Num());
	});

	return FVoxelFloatBuffer::Make(Result);
}