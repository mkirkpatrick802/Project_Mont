// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MarchingCube/VoxelExecNode_OutputMarchingCubePreview.h"
#include "MarchingCube/VoxelMarchingCubeMesh.h"
#include "MarchingCube/VoxelMarchingCubeMeshComponent.h"
#include "MarchingCube/VoxelMarchingCubeCollisionComponent.h"
#include "MarchingCube/VoxelNode_CreateMarchingCubeMesh.h"
#include "MarchingCube/VoxelNode_CreateMarchingCubeCollider.h"
#include "MarchingCube/VoxelNode_GenerateMarchingCubeSurface.h"
#include "VoxelRuntime.h"
#include "VoxelTaskHelpers.h"
#include "VoxelDependencyTracker.h"

TVoxelFutureValue<FVoxelMarchingCubeBrushPreviewMesh> FVoxelExecNode_OutputMarchingCubePreview::CreateMesh(
	const FVoxelQuery& Query,
	const int32 VoxelSize,
	const int32 ChunkSize,
	const FVoxelBox& Bounds) const
{
	checkVoxelSlow(FVoxelTaskReferencer::Get().IsReferenced(this));

	const TValue<FVoxelMarchingCubeSurface> MarchingCubeSurface = VOXEL_CALL_NODE(FVoxelNode_GenerateMarchingCubeSurface, SurfacePin, Query)
	{
		VOXEL_CALL_NODE_BIND(DistancePin)
		{
			const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
			Parameters->Add<FVoxelSurfaceQueryParameter>().ComputeDistance();
			const TValue<FVoxelSurface> Surface = GetNodeRuntime().Get(SurfacePin, Query.MakeNewQuery(Parameters));

			return VOXEL_ON_COMPLETE(Surface)
			{
				return Surface->GetDistance();
			};
		};
		VOXEL_CALL_NODE_BIND(VoxelSizePin, VoxelSize)
		{
			return VoxelSize;
		};
		VOXEL_CALL_NODE_BIND(ChunkSizePin, ChunkSize)
		{
			return ChunkSize;
		};
		VOXEL_CALL_NODE_BIND(BoundsPin, Bounds)
		{
			return Bounds;
		};
		VOXEL_CALL_NODE_BIND(EnableTransitionsPin)
		{
			return false;
		};
		VOXEL_CALL_NODE_BIND(PerfectTransitionsPin)
		{
			return false;
		};
		VOXEL_CALL_NODE_BIND(EnableDistanceChecksPin)
		{
			return false;
		};
		VOXEL_CALL_NODE_BIND(DistanceChecksTolerancePin)
		{
			return 0.f;
		};
	};

	const TValue<FVoxelMarchingCubeMeshWrapper> Mesh = VOXEL_CALL_NODE(FVoxelNode_CreateMarchingCubeMesh, MeshPin, Query)
	{
		VOXEL_CALL_NODE_BIND(SurfacePin, MarchingCubeSurface)
		{
			return MarchingCubeSurface;
		};

		VOXEL_CALL_NODE_BIND(DistancePin)
		{
			const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
			Parameters->Add<FVoxelSurfaceQueryParameter>().ComputeDistance();
			const TValue<FVoxelSurface> Surface = GetNodeRuntime().Get(SurfacePin, Query.MakeNewQuery(Parameters));

			return VOXEL_ON_COMPLETE(Surface)
			{
				return Surface->GetDistance();
			};
		};

		VOXEL_CALL_NODE_BIND(MaterialPin)
		{
			return GetNodeRuntime().Get(MaterialPin, Query);
		};

		VOXEL_CALL_NODE_BIND(GenerateDistanceFieldPin)
		{
			return false;
		};

		VOXEL_CALL_NODE_BIND(DistanceFieldBiasPin)
		{
			return 0.f;
		};

		VOXEL_CALL_NODE_BIND(DistanceFieldSelfShadowBiasPin)
		{
			return 0.f;
		};
	};

	const TValue<FBodyInstance> BodyInstance = GetNodeRuntime().Get(BodyInstancePin, Query);

	const TValue<FVoxelMarchingCubeColliderWrapper> ColliderWrapper =
		Query.GetInfo(EVoxelQueryInfo::Query).FindParameter<FVoxelRuntimeParameter_DisableCollision>()
		? FVoxelMarchingCubeColliderWrapper()
		: VOXEL_CALL_NODE(FVoxelNode_CreateMarchingCubeCollider, ColliderPin, Query)
		{
			VOXEL_CALL_NODE_BIND(SurfacePin, MarchingCubeSurface)
			{
				return MarchingCubeSurface;
			};

			VOXEL_CALL_NODE_BIND(PhysicalMaterialPin)
			{
				return {};
			};
		};

	const TValue<bool> OnlyDrawIfSelected = GetNodeRuntime().Get(OnlyDrawIfSelectedPin, Query);

	return
		MakeVoxelTask()
		.Dependencies(Mesh, BodyInstance, ColliderWrapper, OnlyDrawIfSelected)
		.Execute<FVoxelMarchingCubeBrushPreviewMesh>([=]
		{
			const TSharedRef<FVoxelMarchingCubeBrushPreviewMesh> Result = MakeVoxelShared<FVoxelMarchingCubeBrushPreviewMesh>();
			Result->Bounds = Bounds;
			Result->bOnlyDrawIfSelected = OnlyDrawIfSelected.Get_CheckCompleted();
			Result->Mesh = Mesh.Get_CheckCompleted().Mesh;
			Result->BodyInstance = BodyInstance.GetShared_CheckCompleted();
			Result->Collider = ColliderWrapper.Get_CheckCompleted().Collider;
			return Result;
		});
}

TVoxelUniquePtr<FVoxelExecNodeRuntime> FVoxelExecNode_OutputMarchingCubePreview::CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const
{
	return MakeVoxelUnique<FVoxelExecNodeRuntime_OutputMarchingCubePreview>(SharedThis);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecNodeRuntime_OutputMarchingCubePreview::Create()
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelDynamicValueFactory<FVoxelMarchingCubeBrushPreviewMesh> Factory(STATIC_FNAME("Output Marching Cube Preview"), [&Node = Node](const FVoxelQuery& InQuery)
	{
		checkVoxelSlow(FVoxelTaskReferencer::Get().IsReferenced(&Node));
		const FVoxelQuery Query = InQuery.EnterScope(Node);

		const TValue<int32> FutureSize = Node.GetNodeRuntime().Get(Node.SizePin, Query);
		const TValue<FVoxelBounds> FutureLocalBounds = Node.GetNodeRuntime().Get(Node.BoundsPin, Query);

		return
			MakeVoxelTask()
			.Dependencies(FutureSize, FutureLocalBounds)
			.Execute<FVoxelMarchingCubeBrushPreviewMesh>([=, &Node]
			{
				const TValue<FVoxelSurface> Surface = FutureLocalBounds.Get_CheckCompleted().IsValid()
					? FVoxelSurface()
					: Node.GetNodeRuntime().Get(Node.SurfacePin, Query);

				return
					MakeVoxelTask()
					.Dependency(Surface)
					.Execute<FVoxelMarchingCubeBrushPreviewMesh>([=, &Node]() -> TValue<FVoxelMarchingCubeBrushPreviewMesh>
					{
						const int32 Size = FMath::Clamp(2 * FMath::DivideAndRoundUp(FutureSize.Get_CheckCompleted(), 2), 4, 128);

						FVoxelBounds LocalBoundsRef = FutureLocalBounds.Get_CheckCompleted();
						if (!LocalBoundsRef.IsValid())
						{
							LocalBoundsRef = FVoxelBounds(Surface.Get_CheckCompleted().GetBounds(), Query.GetLocalToWorld());
						}

						if (!LocalBoundsRef.IsValid())
						{
							VOXEL_MESSAGE(Error, "{0}: Invalid bounds", Node);
							return {};
						}
						if (LocalBoundsRef.IsInfinite())
						{
							VOXEL_MESSAGE(Error, "{0}: Infinite bounds", Node);
							return {};
						}

						FVoxelBox LocalBounds = LocalBoundsRef.GetBox(Query, Query.GetQueryToWorld());

						const int32 VoxelSize = FMath::CeilToInt(LocalBounds.Size().GetAbsMax() / Size);
						const double NewSize = VoxelSize * Size;
						const FVector BoundsSize = LocalBounds.Size();

						if (BoundsSize.X < NewSize)
						{
							const double Difference = NewSize - BoundsSize.X;
							LocalBounds.Min.X -= Difference / 2;
							LocalBounds.Max.X += Difference / 2;
						}
						if (BoundsSize.Y < NewSize)
						{
							const double Difference = NewSize - BoundsSize.Y;
							LocalBounds.Min.Y -= Difference / 2;
							LocalBounds.Max.Y += Difference / 2;
						}
						if (BoundsSize.Z < NewSize)
						{
							const double Difference = NewSize - BoundsSize.Z;
							LocalBounds.Min.Z -= Difference / 2;
							LocalBounds.Max.Z += Difference / 2;
						}

						LocalBounds.Min = FVoxelUtilities::FloorToFloat(LocalBounds.Min);
						LocalBounds.Max = LocalBounds.Min + VoxelSize * Size;

						const TSharedRef<FVoxelQueryParameters> LocalParameters = Query.CloneParameters();
						LocalParameters->Add<FVoxelLODQueryParameter>().LOD = 0;

						return Node.CreateMesh(Query.MakeNewQuery(LocalParameters), VoxelSize, Size, LocalBounds);
					});
			});
	});

	MeshValue = Factory
		.AddRef(NodeRef)
		.Compute(GetTerminalGraphInstance());

	MeshValue.OnChanged_GameThread(MakeWeakPtrLambda(this, [this](const TSharedRef<const FVoxelMarchingCubeBrushPreviewMesh>& Mesh)
	{
		const TSharedPtr<FVoxelRuntime> Runtime = GetRuntime();
		if (!ensureVoxelSlow(Runtime))
		{
			return;
		}

		if (!Mesh->Mesh)
		{
			Runtime->DestroyComponent(WeakMeshComponent);
		}
		else
		{
			UVoxelMarchingCubeMeshComponent* Component = WeakMeshComponent.Get();
			if (!Component)
			{
				Component = Runtime->CreateComponent<UVoxelMarchingCubeMeshComponent>();
				WeakMeshComponent = Component;
			}
			if (!ensure(Component))
			{
				return;
			}

			Component->bOnlyDrawIfSelected = Mesh->bOnlyDrawIfSelected;
			Component->SetRelativeLocation(Mesh->Bounds.Min);
			Component->SetMesh(Mesh->Mesh);
		}

		if (!Mesh->Collider)
		{
			Runtime->DestroyComponent(WeakCollisionComponent);
		}
		else
		{
			UVoxelMarchingCubeCollisionComponent* Component = WeakCollisionComponent.Get();
			if (!Component)
			{
				Component = Runtime->CreateComponent<UVoxelMarchingCubeCollisionComponent>();
				WeakCollisionComponent = Component;
			}
			if (!ensure(Component))
			{
				return;
			}

			Component->SetRelativeLocation(Mesh->Collider->Offset);
			Component->SetCollider(Mesh->Collider);

			if (IsGameWorld())
			{
				Component->SetBodyInstance(*Mesh->BodyInstance);
			}
			else
			{
				Component->BodyInstance.SetResponseToAllChannels(ECR_Ignore);
				Component->BodyInstance.SetResponseToChannel(ECC_EngineTraceChannel6, ECR_Block);
			}
		}
	}));
}

void FVoxelExecNodeRuntime_OutputMarchingCubePreview::Destroy()
{
	VOXEL_FUNCTION_COUNTER();

	MeshValue = {};

	if (const TSharedPtr<FVoxelRuntime> Runtime = GetRuntime())
	{
		Runtime->DestroyComponent(WeakMeshComponent);
		Runtime->DestroyComponent(WeakCollisionComponent);
	}
	else
	{
		if (UVoxelMarchingCubeMeshComponent* Component = WeakMeshComponent.Get())
		{
			Component->DestroyComponent();
		}
		if (UVoxelMarchingCubeCollisionComponent* Component = WeakCollisionComponent.Get())
		{
			Component->DestroyComponent();
		}
	}
}

FVoxelOptionalBox FVoxelExecNodeRuntime_OutputMarchingCubePreview::GetBounds() const
{
	VOXEL_FUNCTION_COUNTER();

	FString Error;
	const TOptional<FVoxelBox> Bounds = FVoxelTaskHelpers::TryRunSynchronously(
		GetTerminalGraphInstance(),
		[&]
		{
			const FVoxelQuery Query = FVoxelQuery::Make(
				GetTerminalGraphInstance(),
				MakeVoxelShared<FVoxelQueryParameters>(),
				FVoxelDependencyTracker::Create("DependencyTracker"));

			const TValue<FVoxelBounds> FutureLocalBounds = Node.GetNodeRuntime().Get(Node.BoundsPin, Query);

			return
				MakeVoxelTask()
				.Dependency(FutureLocalBounds)
				.Execute<FVoxelBox>([=]
				{
					const TValue<FVoxelSurface> Surface = FutureLocalBounds.Get_CheckCompleted().IsValid()
						? FVoxelSurface()
						: Node.GetNodeRuntime().Get(Node.SurfacePin, Query);

					return
						MakeVoxelTask()
						.Dependency(Surface)
						.Execute<FVoxelBox>([=]
						{
							FVoxelBounds LocalBounds = FutureLocalBounds.Get_CheckCompleted();
							if (!LocalBounds.IsValid())
							{
								LocalBounds = FVoxelBounds(Surface.Get_CheckCompleted().GetBounds(), Query.GetLocalToWorld());
							}

							// In world space
							return LocalBounds.GetBox_NoDependency(FMatrix::Identity);
						});
				});
		},
		&Error);

	if (!Bounds)
	{
		VOXEL_MESSAGE(Error, "{0}: Failed to query bounds: {1}", Node, Error);
		return {};
	}

	return Bounds.GetValue();
}