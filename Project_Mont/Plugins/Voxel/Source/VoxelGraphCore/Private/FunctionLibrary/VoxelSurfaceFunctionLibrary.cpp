// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "FunctionLibrary/VoxelSurfaceFunctionLibrary.h"
#include "FunctionLibrary/VoxelMathFunctionLibrary.h"
#include "VoxelSurfaceFunctionLibraryImpl.ispc.generated.h"

FVoxelBox UVoxelSurfaceFunctionLibrary::GetSurfaceBounds(const FVoxelSurface& Surface) const
{
	return Surface.GetBounds();
}

FVoxelBounds UVoxelSurfaceFunctionLibrary::MakeBoundsFromLocalBox(const FVoxelBox& Box) const
{
	return FVoxelBounds(Box, GetQuery().GetLocalToWorld());
}

FVoxelBox UVoxelSurfaceFunctionLibrary::GetBoundsBox(
	const FVoxelBounds& Bounds,
	const EVoxelTransformSpace TransformSpace) const
{
	FVoxelTransformRef LocalToWorld;
	switch (TransformSpace)
	{
	default: ensure(false);
	case EVoxelTransformSpace::Local:
	{
		LocalToWorld = GetQuery().GetLocalToWorld();
	}
	break;
	case EVoxelTransformSpace::World:
	{
		LocalToWorld = FVoxelTransformRef::Identity();
	}
	break;
	case EVoxelTransformSpace::Query:
	{
		LocalToWorld = GetQuery().GetQueryToWorld();
	}
	break;
	}

	return Bounds.GetBox(GetQuery(), LocalToWorld);
}

FVoxelMaterialBlendingBuffer UVoxelSurfaceFunctionLibrary::BlendMaterials(
	const FVoxelMaterialBlendingBuffer& A,
	const FVoxelMaterialBlendingBuffer& B,
	const FVoxelFloatBuffer& Alpha) const
{
	const int32 Num = ComputeVoxelBuffersNum_Function(A, B, Alpha);

	FVoxelMaterialBlendingBufferStorage Result;
	Result.Allocate(Num);

	ForeachVoxelBufferChunk_Parallel(Num, [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelSurfaceFunctionLibrary_BlendMaterials(
			ReinterpretCastPtr<uint64>(A.GetData(Iterator)),
			A.IsConstant(),
			ReinterpretCastPtr<uint64>(B.GetData(Iterator)),
			B.IsConstant(),
			Alpha.GetData(Iterator),
			Alpha.IsConstant(),
			Iterator.Num(),
			ReinterpretCastPtr<uint64>(Result.GetData(Iterator)));
	});

	return FVoxelMaterialBlendingBuffer::Make(Result);
}