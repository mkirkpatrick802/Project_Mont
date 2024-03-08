// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MarchingCube/VoxelNode_GenerateMarchingCubeSurface.h"
#include "VoxelPositionQueryParameter.h"
#include "MarchingCube/VoxelMarchingCubeProcessor.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, bool, GVoxelMarchingCubeShowSkippedChunks, false,
	"voxel.marchingcube.ShowSkippedChunks",
	"");

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, bool, GVoxelMarchingCubeShowComputedChunks, false,
	"voxel.marchingcube.ShowComputedChunks",
	"");

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_GenerateMarchingCubeSurface, Surface)
{
	VOXEL_TASK_TAG_SCOPE("Marching Cube Mesh");
	FindVoxelQueryParameter(FVoxelLODQueryParameter, LODQueryParameter);

	const int32 LOD = LODQueryParameter->LOD;

	const TValue<int32> VoxelSize = Get(VoxelSizePin, Query);
	const TValue<int32> ChunkSize = Get(ChunkSizePin, Query);
	const TValue<FVoxelBox> Bounds = Get(BoundsPin, Query);
	const TValue<bool> EnableTransitions = Get(EnableTransitionsPin, Query);
	const TValue<bool> PerfectTransitions = Get(PerfectTransitionsPin, Query);
	const TValue<bool> EnableDistanceChecks = Get(EnableDistanceChecksPin, Query);
	const TValue<float> DistanceChecksTolerance = Get(DistanceChecksTolerancePin, Query);

	return VOXEL_ON_COMPLETE(LOD, VoxelSize, ChunkSize, Bounds, EnableTransitions, PerfectTransitions, EnableDistanceChecks, DistanceChecksTolerance)
	{
		const int32 ScaledVoxelSize = VoxelSize * (1 << LOD);

		if (!ensure(Bounds.Size() == FVector3d(ChunkSize * ScaledVoxelSize)))
		{
			VOXEL_MESSAGE(Error, "{0}: Invalid chunk size", this);
			return {};
		}
		if (!(4 <= ChunkSize && ChunkSize <= 128))
		{
			VOXEL_MESSAGE(Error, "{0}: Invalid chunk size {1}, needs to be between 4 and 128", this, ChunkSize);
			return {};
		}

		const int32 DataSize = ChunkSize + 1;

		const TValue<bool> ShouldSkip = INLINE_LAMBDA -> TValue<bool>
		{
			if (!EnableDistanceChecks)
			{
				return false;
			}

			const float Size = Bounds.Size().GetMax();
			const float Tolerance = FMath::Max(DistanceChecksTolerance, 0.f);

			const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
			Parameters->Add<FVoxelGradientStepQueryParameter>().Step = ScaledVoxelSize;
			Parameters->Add<FVoxelPositionQueryParameter>().InitializeGrid(FVector3f(Bounds.Min) + Size / 4.f, Size / 2.f, FIntVector(2));
			Parameters->Add<FVoxelMinExactDistanceQueryParameter>().MinExactDistance = Size / 4.f * UE_SQRT_2;

			TValue<FVoxelFloatBuffer> Distances;
			{
				VOXEL_TASK_TAG_SCOPE("Distance Checks");
				Distances = Get(DistancePin, Query.MakeNewQuery(Parameters));
			}

			return
				MakeVoxelTask("DistanceChecks")
				.Dependency(Distances)
				.Execute<bool>([=]
				{
					bool bCanSkip = true;
					for (const float Distance : Distances.Get_CheckCompleted())
					{
						if (FMath::Abs(Distance) < Size / 4.f * UE_SQRT_2 * (1.f + Tolerance))
						{
							bCanSkip = false;
						}
					}
					return bCanSkip;
				});
		};

		return VOXEL_ON_COMPLETE(Bounds, LOD, ChunkSize, EnableTransitions, PerfectTransitions, DataSize, ScaledVoxelSize, ShouldSkip)
		{
			if (ShouldSkip)
			{
				if (GVoxelMarchingCubeShowSkippedChunks)
				{
					FVoxelDebugDrawer(Query.GetInfo(EVoxelQueryInfo::Query).GetWorld())
					.Color(FColor::Green)
					.Thickness(5.f)
					.DrawBox(Bounds, Query.GetQueryToWorld().Get_NoDependency());
				}
				return {};
			}

			if (GVoxelMarchingCubeShowComputedChunks)
			{
				FVoxelDebugDrawer(Query.GetInfo(EVoxelQueryInfo::Query).GetWorld())
				.Color(FColor::Red)
				.Thickness(5.f)
				.DrawBox(Bounds, Query.GetQueryToWorld().Get_NoDependency());
			}

			const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
			Parameters->Add<FVoxelGradientStepQueryParameter>().Step = ScaledVoxelSize;
			Parameters->Add<FVoxelPositionQueryParameter>().InitializeGrid(FVector3f(Bounds.Min), ScaledVoxelSize, FIntVector(DataSize));

			TValue<FVoxelFloatBuffer> Distances;
			{
				VOXEL_TASK_TAG_SCOPE("Distance");
				Distances = Get(DistancePin, Query.MakeNewQuery(Parameters));
			}

			return VOXEL_ON_COMPLETE(Bounds, LOD, ChunkSize, EnableTransitions, PerfectTransitions, DataSize, ScaledVoxelSize, Distances)
			{
				if (Distances.IsConstant() ||
					!ensure(Distances.Num() == DataSize * DataSize * DataSize))
				{
					return {};
				}

				const TSharedRef<FVoxelMarchingCubeSurface> Surface = MakeVoxelShared<FVoxelMarchingCubeSurface>();
				Surface->LOD = LOD;
				Surface->ChunkSize = ChunkSize;
				Surface->ScaledVoxelSize = ScaledVoxelSize;
				Surface->ChunkBounds = Bounds;

				const TSharedRef<FVoxelMarchingCubeProcessor> Processor = MakeVoxelShared<FVoxelMarchingCubeProcessor>(
					ChunkSize,
					DataSize,
					ConstCast(Distances.GetStorage()),
					*Surface);
				Processor->bPerfectTransitions = PerfectTransitions;
				Processor->Generate(EnableTransitions);

				if (Surface->Cells.Num() == 0)
				{
					return {};
				}
				ensure(Surface->Indices.Num() > 0);

				if (!PerfectTransitions)
				{
					return Surface;
				}

				using FTransitionVertexToQuery = FVoxelMarchingCubeProcessor::FTransitionVertexToQuery;

				FVoxelFloatBufferStorage QueryX;
				FVoxelFloatBufferStorage QueryY;
				FVoxelFloatBufferStorage QueryZ;

				for (const TVoxelArray<FTransitionVertexToQuery>& Array : Processor->TransitionVerticesToQuery)
				{
					for (const FTransitionVertexToQuery& VertexToQuery : Array)
					{
						QueryX.Add(float(Bounds.Min.X) + VertexToQuery.PositionA.X * ScaledVoxelSize);
						QueryY.Add(float(Bounds.Min.Y) + VertexToQuery.PositionA.Y * ScaledVoxelSize);
						QueryZ.Add(float(Bounds.Min.Z) + VertexToQuery.PositionA.Z * ScaledVoxelSize);

						QueryX.Add(float(Bounds.Min.X) + VertexToQuery.PositionB.X * ScaledVoxelSize);
						QueryY.Add(float(Bounds.Min.Y) + VertexToQuery.PositionB.Y * ScaledVoxelSize);
						QueryZ.Add(float(Bounds.Min.Z) + VertexToQuery.PositionB.Z * ScaledVoxelSize);
					}
				}

				if (QueryX.Num() == 0 ||
					QueryY.Num() == 0 ||
					QueryZ.Num() == 0)
				{
					ensure(QueryX.Num() == 0);
					ensure(QueryY.Num() == 0);
					ensure(QueryZ.Num() == 0);
					return Surface;
				}

				const TSharedRef<FVoxelQueryParameters> TransitionParameters = Query.CloneParameters();
				TransitionParameters->Add<FVoxelLODQueryParameter>().LOD = LOD + 1;
				TransitionParameters->Add<FVoxelGradientStepQueryParameter>().Step = ScaledVoxelSize * 2;
				TransitionParameters->Add<FVoxelPositionQueryParameter>().Initialize(FVoxelVectorBuffer::Make(QueryX, QueryY, QueryZ));

				const TValue<FVoxelFloatBuffer> TransitionDistances = Get(DistancePin, Query.MakeNewQuery(TransitionParameters));

				return VOXEL_ON_COMPLETE(Surface, Processor, TransitionDistances)
				{
					int32 Index = 0;
					for (int32 Direction = 0; Direction < 6; Direction++)
					{
						for (const FTransitionVertexToQuery& VertexToQuery : Processor->TransitionVerticesToQuery[Direction])
						{
							const float ValueA = TransitionDistances[Index++];
							const float ValueB = TransitionDistances[Index++];

							const float Alpha = ValueA / (ValueA - ValueB);
							const FVector3f Position = FMath::Lerp(FVector3f(VertexToQuery.PositionA), FVector3f(VertexToQuery.PositionB), Alpha);

							Surface->TransitionVertices[Direction][VertexToQuery.Index].Position = Position;
						}
					}
					ensure(TransitionDistances.IsConstant() || TransitionDistances.Num() == Index);

					return Surface;
				};
			};
		};
	};
}