// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelSurfaceNodes.h"
#include "VoxelGraphMigration.h"
#include "VoxelPositionQueryParameter.h"
#include "VoxelSurfaceNodesImpl.ispc.generated.h"

VOXEL_RUN_ON_STARTUP_GAME(VoxelSurfaceNodeMigrations)
{
	REGISTER_VOXEL_NODE_MIGRATION("Invert", FVoxelNode_InvertSurface);
	REGISTER_VOXEL_NODE_PIN_MIGRATION(FVoxelNode_InvertSurface, ReturnValuePin, NewSurfacePin)
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_InvertSurface, NewSurface)
{
	const TValue<FVoxelSurface> Surface = Get(SurfacePin, Query);

	return VOXEL_ON_COMPLETE(Surface)
	{
		const TSharedRef<FVoxelSurface> NewSurface = MakeSharedCopy(*Surface);
		NewSurface->SetBounds(FVoxelBox::Infinite);

		FindOptionalVoxelQueryParameter(FVoxelSurfaceQueryParameter, SurfaceQueryParameter);

		if (!SurfaceQueryParameter ||
			!SurfaceQueryParameter->ShouldComputeDistance() ||
			!Surface->HasDistance())
		{
			// No need to actually invert
			return NewSurface;
		}
		const FVoxelFloatBuffer Distance = NewSurface->GetDistance();

		FVoxelFloatBufferStorage NewDistance;
		NewDistance.Allocate(Distance.Num());

		ForeachVoxelBufferChunk_Parallel("VoxelNode_InvertSurface", Distance.Num(), [&](const FVoxelBufferIterator& Iterator)
		{
			ispc::VoxelNode_InvertSurface(
				Distance.GetData(Iterator),
				NewDistance.GetData(Iterator),
				Iterator.Num());
		});

		NewSurface->SetDistance(FVoxelFloatBuffer::Make(NewDistance));

		return NewSurface;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_MaskSurface, NewSurface)
{
	const TValue<FVoxelSurface> Surface = Get(SurfacePin, Query);

	return VOXEL_ON_COMPLETE(Surface)
	{
		FindOptionalVoxelQueryParameter(FVoxelSurfaceQueryParameter, SurfaceQueryParameter);

		if (!SurfaceQueryParameter ||
			!SurfaceQueryParameter->ShouldComputeDistance() ||
			!Surface->HasDistance())
		{
			// No need to actually mask
			return Surface;
		}

		const TValue<FVoxelFloatBuffer> MaskDistance = Get(MaskDistancePin, Query);
		const TValue<float> Smoothness = Get(SmoothnessPin, Query);

		return VOXEL_ON_COMPLETE(Surface, MaskDistance, Smoothness)
		{
			const FVoxelFloatBuffer Distance = Surface->GetDistance();
			const int32 Num = ComputeVoxelBuffersNum(Distance, MaskDistance);

			FVoxelFloatBufferStorage NewDistance;
			NewDistance.Allocate(Num);

			ForeachVoxelBufferChunk_Parallel("VoxelNode_MaskSurface", Num, [&](const FVoxelBufferIterator& Iterator)
			{
				ispc::VoxelNode_MaskSurface(
					Distance.GetData(Iterator),
					Distance.IsConstant(),
					MaskDistance.GetData(Iterator),
					MaskDistance.IsConstant(),
					Smoothness,
					NewDistance.GetData(Iterator),
					Iterator.Num());
			});

			const TSharedRef<FVoxelSurface> NewSurface = MakeSharedCopy(*Surface);
			NewSurface->SetDistance(FVoxelFloatBuffer::Make(NewDistance));
			return NewSurface;
		};
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_InflateSurface, NewSurface)
{
	const TValue<FVoxelSurface> Surface = Get(SurfacePin, Query);
	const TValue<float> Amount = Get(AmountPin, Query);

	return VOXEL_ON_COMPLETE(Surface, Amount)
	{
		const TSharedRef<FVoxelSurface> NewSurface = MakeSharedCopy(*Surface);
		if (Surface->GetBounds().IsValid() &&
			Amount > 0)
		{
			NewSurface->SetBounds(Surface->GetBounds().Extend(Amount));
		}

		FindOptionalVoxelQueryParameter(FVoxelSurfaceQueryParameter, SurfaceQueryParameter);

		if (!SurfaceQueryParameter ||
			!SurfaceQueryParameter->ShouldComputeDistance() ||
			!Surface->HasDistance())
		{
			// No need to actually inflate
			return NewSurface;
		}

		const FVoxelFloatBuffer Distance = Surface->GetDistance();

		FVoxelFloatBufferStorage NewDistance;
		NewDistance.Allocate(Distance.Num());

		ForeachVoxelBufferChunk_Parallel("VoxelNode_InflateSurface", Distance.Num(), [&](const FVoxelBufferIterator& Iterator)
		{
			ispc::VoxelNode_InflateSurface(
				Distance.GetData(Iterator),
				Distance.IsConstant(),
				Amount,
				NewDistance.GetData(Iterator),
				Iterator.Num());
		});

		NewSurface->SetDistance(FVoxelFloatBuffer::Make(NewDistance));
		return NewSurface;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_DisplaceSurface, NewSurface)
{
	const TValue<float> MaxDisplacement = Get(MaxDisplacementPin, Query);
	const TValue<FVoxelVectorBuffer> Displacement = Get(DisplacementPin, Query);

	return VOXEL_ON_COMPLETE(MaxDisplacement, Displacement)
	{
		if (MaxDisplacement <= 0)
		{
			return Get(SurfacePin, Query);
		}

		FindVoxelQueryParameter(FVoxelPositionQueryParameter, PositionQueryParameter);
		const FVoxelVectorBuffer Positions = PositionQueryParameter->GetPositions();
		const int32 Num = ComputeVoxelBuffersNum(Positions, Displacement);

		FVoxelFloatBufferStorage NewPositionX; NewPositionX.Allocate(Num);
		FVoxelFloatBufferStorage NewPositionY; NewPositionY.Allocate(Num);
		FVoxelFloatBufferStorage NewPositionZ; NewPositionZ.Allocate(Num);

		ForeachVoxelBufferChunk_Parallel("VoxelNode_DisplaceSurface", Num, [&](const FVoxelBufferIterator& Iterator)
		{
			ispc::VoxelNode_DisplaceSurface(
				Displacement.X.GetData(Iterator),
				Displacement.X.IsConstant(),
				Displacement.Y.GetData(Iterator),
				Displacement.Y.IsConstant(),
				Displacement.Z.GetData(Iterator),
				Displacement.Z.IsConstant(),
				Positions.X.GetData(Iterator),
				Positions.X.IsConstant(),
				Positions.Y.GetData(Iterator),
				Positions.Y.IsConstant(),
				Positions.Z.GetData(Iterator),
				Positions.Z.IsConstant(),
				MaxDisplacement,
				NewPositionX.GetData(Iterator),
				NewPositionY.GetData(Iterator),
				NewPositionZ.GetData(Iterator),
				Iterator.Num());
		});

		const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
		Parameters->Add<FVoxelPositionQueryParameter>().Initialize(FVoxelVectorBuffer::Make(
			NewPositionX,
			NewPositionY,
			NewPositionZ));

		const TValue<FVoxelSurface> Surface = Get(SurfacePin, Query.MakeNewQuery(Parameters));

		return VOXEL_ON_COMPLETE(MaxDisplacement, Surface)
		{
			const TSharedRef<FVoxelSurface> NewSurface = MakeSharedCopy(*Surface);
			if (Surface->GetBounds().IsValid())
			{
				NewSurface->SetBounds(Surface->GetBounds().Extend(MaxDisplacement));
			}
			return NewSurface;
		};
	};
}