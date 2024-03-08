// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelNode_CreateHeightDistanceField.h"
#include "VoxelGraphMigration.h"
#include "VoxelPositionQueryParameter.h"
#include "VoxelNode_CreateHeightDistanceFieldImpl.ispc.generated.h"

VOXEL_RUN_ON_STARTUP_GAME(VoxelDistanceNodesMigration)
{
	REGISTER_VOXEL_NODE_MIGRATION("Make Height Surface", FVoxelNode_CreateHeightDistanceField);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_CreateHeightDistanceField, Distance)
{
	FindVoxelQueryParameter(FVoxelPositionQueryParameter, PositionQueryParameter);

	const TValue<FVoxelFloatBuffer> Height = INLINE_LAMBDA -> TValue<FVoxelFloatBuffer>
	{
		if (!PositionQueryParameter->IsGrid())
		{
			return Get(HeightPin, Query);
		}

		const int32 SizeZ = PositionQueryParameter->GetGrid().Size.Z;
		if (SizeZ <= 1)
		{
			return Get(HeightPin, Query);
		}

		FIntVector Size = PositionQueryParameter->GetGrid().Size;
		Size.Z = 1;

		const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
		Parameters->Add<FVoxelPositionQueryParameter>().InitializeGrid(
			PositionQueryParameter->GetGrid().Start,
			PositionQueryParameter->GetGrid().Step,
			Size);
		const TValue<FVoxelFloatBuffer> LocalHeights = Get(HeightPin, Query.MakeNewQuery(Parameters));

		return
			MakeVoxelTask("UnpackHeights")
			.Dependency(LocalHeights)
			.Execute<FVoxelFloatBuffer>([=]
			{
				FVoxelFloatBufferStorage Result;
				for (int32 Z = 0; Z < SizeZ; Z++)
				{
					Result.Append(
						LocalHeights.Get_CheckCompleted().GetStorage(),
						Size.X * Size.Y);
				}
				return FVoxelFloatBuffer::Make(Result);
			});
	};

	return VOXEL_ON_COMPLETE(PositionQueryParameter, Height)
	{
		const FVoxelFloatBuffer Z = PositionQueryParameter->GetPositions().Z;
		const FVoxelBufferAccessor BufferAccessor(Height, Z);
		if (!BufferAccessor.IsValid())
		{
			RaiseBufferError();
			return {};
		}

		FVoxelFloatBufferStorage Distance;
		Distance.Allocate(BufferAccessor.Num());

		ForeachVoxelBufferChunk_Parallel(BufferAccessor.Num(), [&](const FVoxelBufferIterator& Iterator)
		{
			ispc::VoxelNode_CreateHeightDistanceField(
				Height.GetData(Iterator),
				Height.IsConstant(),
				Z.GetData(Iterator),
				Z.IsConstant(),
				Iterator.Num(),
				Distance.GetData(Iterator));
		});

		return FVoxelFloatBuffer::Make(Distance);
	};
}