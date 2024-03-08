// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MarchingCube/VoxelNode_CreateMarchingCubeMesh.h"
#include "VoxelDistanceFieldWrapper.h"
#include "VoxelPositionQueryParameter.h"
#include "Nodes/VoxelNode_GetGradient.h"
#include "Nodes/VoxelDetailTextureNodes.h"
#include "MarchingCube/VoxelMarchingCubeMesh.h"

// UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2=1 is broken for MeshCardBuild.h
#undef UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2
#define UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2 0
#include "MeshCardBuild.h"
#include "DistanceFieldAtlas.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, int32, GVoxelMarchingCubeNumDistanceFieldBricksPerChunk, 4,
	"voxel.marchingcube.NumDistanceFieldBricksPerChunk",
	"");

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, bool, GVoxelMarchingCubeEnableDistanceFieldPadding, false,
	"voxel.marchingcube.EnableDistanceFieldPadding",
	"Add padding to perfectly overlap chunks distance fields. "
	"This might cause invalid entries into Lumen's surface cache and glitches in Lumen at chunk borders.");

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_CreateMarchingCubeMesh, Mesh)
{
	const TValue<FVoxelMarchingCubeSurface> Surface = Get(SurfacePin, Query);
	return VOXEL_ON_COMPLETE(Surface)
	{
		if (Surface->Vertices.Num() == 0)
		{
			return {};
		}

		const TValue<bool> GenerateDistanceField = Get(GenerateDistanceFieldPin, Query);
		const TValue<float> DistanceFieldBias = Get(DistanceFieldBiasPin, Query);
		const TValue<float> DistanceFieldSelfShadowBias = Get(DistanceFieldSelfShadowBiasPin, Query);
		const TValue<FVoxelMaterial> Material = Get(MaterialPin, Query);

		return VOXEL_ON_COMPLETE(Surface, GenerateDistanceField, DistanceFieldBias, DistanceFieldSelfShadowBias, Material)
		{
			FindVoxelQueryParameter(FVoxelLODQueryParameter, LODQueryParameter);

			const TSharedRef<FVoxelDetailTextureQueryHelper> DetailTextureHelper = MakeVoxelShared<FVoxelDetailTextureQueryHelper>(Surface);

			const TValue<FVoxelComputedMaterial> ComputedMaterial = INLINE_LAMBDA
			{
				VOXEL_TASK_TAG_SCOPE("Material");

				const TSharedRef<FVoxelQueryParameters> MaterialParameters = Query.CloneParameters();
				MaterialParameters->Add<FVoxelDetailTextureQueryParameter>().Helper = DetailTextureHelper;
				return Material->Compute(Query.MakeNewQuery(MaterialParameters));
			};

			const FVoxelBox Bounds = FVoxelBox::FromPositions(Surface->Vertices);
			ensure(Bounds.IsValid());

			const int32 NumVertexNormals = LODQueryParameter->LOD == 0 ? Surface->Vertices.Num() : Surface->NumEdgeVertices;

			TValue<FVoxelVectorBuffer> VertexNormals;
			if (NumVertexNormals > 0)
			{
				VOXEL_TASK_TAG_SCOPE("Vertex Normals");

				const float Scale = Surface->ScaledVoxelSize;
				const FVector3f Offset = FVector3f(Surface->ChunkBounds.Min);

				const TSharedRef<FVoxelQueryParameters> VertexNormalParameters = Query.CloneParameters();
				VertexNormalParameters->Add<FVoxelGradientStepQueryParameter>().Step = Surface->ScaledVoxelSize;
				VertexNormalParameters->Add<FVoxelPositionQueryParameter>().Initialize([this, Query, Surface, NumVertexNormals, Scale, Offset]
				{
					const TVoxelArray<FVector3f>& Vertices = Surface->Vertices;

					FVoxelFloatBufferStorage X; X.Allocate(NumVertexNormals);
					FVoxelFloatBufferStorage Y; Y.Allocate(NumVertexNormals);
					FVoxelFloatBufferStorage Z; Z.Allocate(NumVertexNormals);

					for (int32 Index = 0; Index < NumVertexNormals; Index++)
					{
						const FVector3f Vertex = Vertices[Index];
						X[Index] = Offset.X + Scale * Vertex.X;
						Y[Index] = Offset.Y + Scale * Vertex.Y;
						Z[Index] = Offset.Z + Scale * Vertex.Z;
					}

					return FVoxelVectorBuffer::Make(X, Y, Z);
				}, Bounds.Scale(Scale).ShiftBy(FVector(Offset)));

				VertexNormals = VOXEL_CALL_NODE(FVoxelNode_GetGradient, GradientPin, Query.MakeNewQuery(VertexNormalParameters))
				{
					VOXEL_CALL_NODE_BIND(ValuePin)
					{
						return GetNodeRuntime().Get(DistancePin, Query);
					};
				};
			}
			else
			{
				VertexNormals = FVoxelVectorBuffer();
			}

			checkStatic(DistanceField::MeshDistanceFieldObjectBorder == 1);

			const float ChunkWorldSize = Surface->ChunkSize * Surface->ScaledVoxelSize;
			const int32 NumBricksPerChunk = GVoxelMarchingCubeNumDistanceFieldBricksPerChunk;
			const int32 NumQueriesPerChunk = NumBricksPerChunk * DistanceField::UniqueDataBrickSize;
			// 2 * MeshDistanceFieldObjectBorder, additional padding at the end to overlap chunks
			const int32 NumQueriesInChunk = NumQueriesPerChunk - 2 - (GVoxelMarchingCubeEnableDistanceFieldPadding ? 1 : 0);
			const float TexelSize = ChunkWorldSize / float(NumQueriesInChunk);

			// +1 so we can query the last brick element
			// +1 to be a multiple of 2
			const int32 DenseQuerySize = NumQueriesPerChunk + 2;
			const FVoxelBox QueryBounds(0, ChunkWorldSize + (GVoxelMarchingCubeEnableDistanceFieldPadding ? TexelSize : 0));

			TValue<FVoxelFloatBuffer> Distances;
			if (GenerateDistanceField)
			{
				VOXEL_TASK_TAG_SCOPE("Distance Field");

				const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
				Parameters->Add<FVoxelGradientStepQueryParameter>().Step = TexelSize;

				Parameters->Add<FVoxelPositionQueryParameter>().InitializeGrid(
					FVector3f(Surface->ChunkBounds.Min + QueryBounds.Min - TexelSize),
					TexelSize,
					FIntVector(DenseQuerySize));

				Distances = Get(DistancePin, Query.MakeNewQuery(Parameters));
			}
			else
			{
				Distances = FVoxelFloatBuffer();
			}

			return VOXEL_ON_COMPLETE(
				Surface,
				GenerateDistanceField,
				DistanceFieldBias,
				DistanceFieldSelfShadowBias,
				LODQueryParameter,
				DetailTextureHelper,
				ComputedMaterial,
				Bounds,
				NumVertexNormals,
				VertexNormals,
				QueryBounds,
				NumBricksPerChunk,
				DenseQuerySize,
				Distances)
			{
				const TSharedRef<FVoxelMarchingCubeMesh> Mesh = FVoxelMarchingCubeMesh::New();
				Mesh->LOD = LODQueryParameter->LOD;
				Mesh->Bounds = Bounds.Scale(Surface->ScaledVoxelSize);
				Mesh->VoxelSize = Surface->ScaledVoxelSize;
				Mesh->ChunkSize = Surface->ChunkSize;
				Mesh->NumCells = Surface->Cells.Num();
				Mesh->NumEdgeVertices = Surface->NumEdgeVertices;
				Mesh->ComputedMaterial = ComputedMaterial;

				Mesh->Indices = Surface->Indices;
				Mesh->Vertices = Surface->Vertices;
				Mesh->CellIndices = Surface->CellIndices;

				Mesh->TransitionIndices = Surface->TransitionIndices;
				Mesh->TransitionVertices = Surface->TransitionVertices;
				Mesh->TransitionCellIndices = Surface->TransitionCellIndices;

				Mesh->CellIndexToDirection = DetailTextureHelper->BuildCellIndexToDirection();
				Mesh->CellTextureCoordinates = DetailTextureHelper->BuildCellCoordinates();

				if (VertexNormals.Num() > 0 &&
					ensure(VertexNormals.IsConstant() || VertexNormals.Num() == NumVertexNormals))
				{
					Mesh->bHasVertexNormals = NumVertexNormals == Surface->Vertices.Num();

					FVoxelUtilities::SetNumFast(Mesh->VertexNormals, NumVertexNormals);

					for (int32 Index = 0; Index < NumVertexNormals; Index++)
					{
						Mesh->VertexNormals[Index] = VertexNormals[Index].GetSafeNormal();
					}
				}

				if (GenerateDistanceField)
				{
					{
						VOXEL_SCOPE_COUNTER("Cards");

						const FVoxelBox CardBounds = Bounds.Scale(Surface->ScaledVoxelSize);

						Mesh->CardRepresentationData = MakeVoxelShared<FCardRepresentationData>();
						Mesh->CardRepresentationData->MeshCardsBuildData.Bounds = CardBounds.ToFBox();

						for (int32 Index = 0; Index < 6; Index++)
						{
							FLumenCardBuildData& CardBuildData = Mesh->CardRepresentationData->MeshCardsBuildData.CardBuildData.Emplace_GetRef();

							const uint32 AxisIndex = Index / 2;
							FVector3f Direction(0.0f, 0.0f, 0.0f);
							Direction[AxisIndex] = Index & 1 ? 1.0f : -1.0f;

							CardBuildData.OBB.AxisZ = Direction;
							CardBuildData.OBB.AxisZ.FindBestAxisVectors(CardBuildData.OBB.AxisX, CardBuildData.OBB.AxisY);
							CardBuildData.OBB.AxisX = FVector3f::CrossProduct(CardBuildData.OBB.AxisZ, CardBuildData.OBB.AxisY);
							CardBuildData.OBB.AxisX.Normalize();

							CardBuildData.OBB.Origin = FVector3f(CardBounds.GetCenter());
							CardBuildData.OBB.Extent = CardBuildData.OBB.RotateLocalToCard(FVector3f(CardBounds.GetExtent()) + FVector3f(1.0f)).GetAbs();

							CardBuildData.AxisAlignedDirectionIndex = Index;
						}
					}

					VOXEL_SCOPE_COUNTER("Distance Field");

					FVoxelDistanceFieldWrapper Wrapper(QueryBounds.ToFBox());
					Wrapper.SetSize(FIntVector(NumBricksPerChunk));

					const FVoxelIntBox BrickBoundsA(0, NumBricksPerChunk);
					const FVoxelIntBox BrickBoundsB(0, DistanceField::BrickSize);

					BrickBoundsA.Iterate([&](const FIntVector& BrickPositionA)
					{
						FVoxelDistanceFieldWrapper::FMip& Mip = Wrapper.Mips[0];
						FVoxelDistanceFieldWrapper::FBrick& Brick = Mip.FindOrAddBrick(BrickPositionA);
						BrickBoundsB.Iterate([&](const FIntVector& BrickPositionB)
						{
							// 7 unique voxel, and 1 voxel shared with the next brick
							// Shader samples between [0.5, 7.5] for good interpolation
							const FIntVector DistancePosition = BrickPositionA * DistanceField::UniqueDataBrickSize + BrickPositionB;
							const int32 DistanceIndex = FVoxelUtilities::Get3DIndex<int32>(DenseQuerySize, DistancePosition);
							const float Distance = Distances[DistanceIndex] + DistanceFieldBias;

							const int32 BrickIndex = FVoxelUtilities::Get3DIndex<int32>(DistanceField::BrickSize, BrickPositionB);
							Brick[BrickIndex] = Mip.QuantizeDistance(Distance);
						});
					});

					Wrapper.Mips[1] = Wrapper.Mips[0];
					Wrapper.Mips[2] = Wrapper.Mips[0];

					Mesh->DistanceFieldVolumeData = Wrapper.Build();
					Mesh->SelfShadowBias = DistanceFieldSelfShadowBias;
					//Mesh->DistanceFieldVolumeData->LocalSpaceMeshBounds = Mesh->DistanceFieldVolumeData->LocalSpaceMeshBounds.ShiftBy(-FVector(Surface->ScaledVoxelSize / 2.f));
				}

				const TSharedRef<FVoxelMarchingCubeMeshWrapper> Wrapper = MakeVoxelShared<FVoxelMarchingCubeMeshWrapper>();
				Wrapper->Mesh = Mesh;
				return Wrapper;
			};
		};
	};
}