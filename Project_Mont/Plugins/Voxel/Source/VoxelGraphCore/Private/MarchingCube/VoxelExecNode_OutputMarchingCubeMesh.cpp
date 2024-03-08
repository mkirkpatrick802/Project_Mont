// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MarchingCube/VoxelExecNode_OutputMarchingCubeMesh.h"
#include "MarchingCube/VoxelExecNodeRuntime_OutputMarchingCubeMesh.h"
#include "MarchingCube/VoxelNode_CreateMarchingCubeMesh.h"
#include "MarchingCube/VoxelNode_CreateMarchingCubeCollider.h"
#include "MarchingCube/VoxelNode_GenerateMarchingCubeSurface.h"
#include "Nodes/VoxelNode_Debug.h"
#include "Nodes/VoxelNode_GetGradient.h"
#include "Nodes/VoxelDetailTextureNodes.h"
#include "Material/VoxelMaterialBlending.h"
#include "VoxelSettings.h"

TVoxelFutureValue<FVoxelMarchingCubeExecNodeMesh> FVoxelExecNode_OutputMarchingCubeMesh::CreateMesh(
	const FVoxelQuery& InQuery,
	const int32 VoxelSize,
	const int32 ChunkSize,
	const FVoxelBox& Bounds) const
{
	checkVoxelSlow(FVoxelTaskReferencer::Get().IsReferenced(this));

	const TSharedRef<FVoxelDebugQueryParameter> DebugParameter = MakeVoxelShared<FVoxelDebugQueryParameter>();

	const FVoxelQuery Query = INLINE_LAMBDA
	{
		const TSharedRef<FVoxelQueryParameters> Parameters = InQuery.CloneParameters();
		Parameters->Add(DebugParameter);
		return InQuery.EnterScope(*this).MakeNewQuery(Parameters);
	};

	const TValue<FVoxelMarchingCubeSurface> MarchingCubeSurface = VOXEL_CALL_NODE(FVoxelNode_GenerateMarchingCubeSurface, SurfacePin, Query)
	{
		VOXEL_CALL_NODE_BIND(DistancePin)
		{
			const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
			Parameters->Add<FVoxelSurfaceQueryParameter>().ComputeDistance();
			const TValue<FVoxelSurface> Surface = GetSurface(Query.MakeNewQuery(Parameters));

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
			return true;
		};
		VOXEL_CALL_NODE_BIND(PerfectTransitionsPin)
		{
			return GetNodeRuntime().Get(PerfectTransitionsPin, Query);
		};
		VOXEL_CALL_NODE_BIND(EnableDistanceChecksPin)
		{
			return true;
		};
		VOXEL_CALL_NODE_BIND(DistanceChecksTolerancePin)
		{
			return GetNodeRuntime().Get(DistanceChecksTolerancePin, Query);
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
			const TValue<FVoxelSurface> Surface = GetSurface(Query.MakeNewQuery(Parameters));

			return VOXEL_ON_COMPLETE(Surface)
			{
				return Surface->GetDistance();
			};
		};

		VOXEL_CALL_NODE_BIND(MaterialPin, DebugParameter)
		{
			FindVoxelQueryParameter(FVoxelLODQueryParameter, LODQueryParameter);

			const TValue<FVoxelMaterialDetailTextureRef> FutureMaterialDetailTexture = GetNodeRuntime().Get(MaterialDetailTexturePin, Query);

			const TValue<FVoxelMaterialParameter> FutureMaterialMaterialParameter =
				MakeVoxelTask()
				.Dependency(FutureMaterialDetailTexture)
				.Execute<FVoxelMaterialParameter>([=]() -> TValue<FVoxelMaterialParameter>
				{
					if (!FutureMaterialDetailTexture.Get_CheckCompleted().WeakDetailTexture.IsValid())
					{
						return {};
					}

					return
						VOXEL_CALL_NODE(FVoxelNode_MakeMaterialDetailTextureParameter, ParameterPin, Query)
						{
							CalleeNode.bIsMain = true;

							VOXEL_CALL_NODE_BIND(MaterialPin)
							{
								const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
								Parameters->Add<FVoxelSurfaceQueryParameter>().Compute(FVoxelSurfaceAttributes::Material);
								const TValue<FVoxelSurface> Surface = GetSurface(Query.MakeNewQuery(Parameters));

								return VOXEL_ON_COMPLETE(Surface)
								{
									return Surface->Get<FVoxelMaterialBlending>(FVoxelSurfaceAttributes::Material);
								};
							};
							VOXEL_CALL_NODE_BIND(TexturePin, FutureMaterialDetailTexture)
							{
								return FutureMaterialDetailTexture;
							};
						};
				});

			const int32 LOD = LODQueryParameter->LOD;
			const TValue<FVoxelMaterial> MaterialTemplate = INLINE_LAMBDA
			{
				VOXEL_SCOPE_LOCK(DebugParameter->CriticalSection);

				if (DebugParameter->Entries_RequiresLock.Num() == 0)
				{
					return GetNodeRuntime().Get(MaterialPin, Query);
				}

				TVoxelArray<FVoxelGraphNodeRef> NodeRefs;
				TVoxelArray<TSharedPtr<const TVoxelComputeValue<FVoxelFloatBuffer>>> ChannelComputes;
				for (const auto& It : DebugParameter->Entries_RequiresLock)
				{
					const FVoxelDebugQueryParameter::FEntry& Entry = It.Value;
					if (Entry.Type.Is<FVoxelFloatBuffer>())
					{
						NodeRefs.Add(Entry.NodeRef);
						ChannelComputes.Add(ReinterpretCastSharedPtr<const TVoxelComputeValue<FVoxelFloatBuffer>>(Entry.Compute));
					}
					else if (Entry.Type.Is<FVoxelVector2DBuffer>())
					{
						NodeRefs.Add(Entry.NodeRef);
						NodeRefs.Add(Entry.NodeRef);

						ChannelComputes.Add(MakeVoxelShared<TVoxelComputeValue<FVoxelFloatBuffer>>([Compute = Entry.Compute](const FVoxelQuery& Query)
						{
							const FVoxelFutureValue Value = (*Compute)(Query);
							return
								MakeVoxelTask()
								.Dependency(Value)
								.Execute<FVoxelFloatBuffer>([=]
								{
									return Value.Get_CheckCompleted<FVoxelVector2DBuffer>().X;
								});
						}));
						ChannelComputes.Add(MakeVoxelShared<TVoxelComputeValue<FVoxelFloatBuffer>>([Compute = Entry.Compute](const FVoxelQuery& Query)
						{
							const FVoxelFutureValue Value = (*Compute)(Query);
							return
								MakeVoxelTask()
								.Dependency(Value)
								.Execute<FVoxelFloatBuffer>([=]
								{
									return Value.Get_CheckCompleted<FVoxelVector2DBuffer>().Y;
								});
						}));
					}
					else if (Entry.Type.Is<FVoxelVectorBuffer>())
					{
						NodeRefs.Add(Entry.NodeRef);
						NodeRefs.Add(Entry.NodeRef);
						NodeRefs.Add(Entry.NodeRef);

						ChannelComputes.Add(MakeVoxelShared<TVoxelComputeValue<FVoxelFloatBuffer>>([Compute = Entry.Compute](const FVoxelQuery& Query)
						{
							const FVoxelFutureValue Value = (*Compute)(Query);
							return
								MakeVoxelTask()
								.Dependency(Value)
								.Execute<FVoxelFloatBuffer>([=]
								{
									return Value.Get_CheckCompleted<FVoxelVectorBuffer>().X;
								});
						}));
						ChannelComputes.Add(MakeVoxelShared<TVoxelComputeValue<FVoxelFloatBuffer>>([Compute = Entry.Compute](const FVoxelQuery& Query)
						{
							const FVoxelFutureValue Value = (*Compute)(Query);
							return
								MakeVoxelTask()
								.Dependency(Value)
								.Execute<FVoxelFloatBuffer>([=]
								{
									return Value.Get_CheckCompleted<FVoxelVectorBuffer>().Y;
								});
						}));
						ChannelComputes.Add(MakeVoxelShared<TVoxelComputeValue<FVoxelFloatBuffer>>([Compute = Entry.Compute](const FVoxelQuery& Query)
						{
							const FVoxelFutureValue Value = (*Compute)(Query);
							return
								MakeVoxelTask()
								.Dependency(Value)
								.Execute<FVoxelFloatBuffer>([=]
								{
									return Value.Get_CheckCompleted<FVoxelVectorBuffer>().Z;
								});
						}));
					}
				}

				if (ChannelComputes.Num() == 0)
				{
					return GetNodeRuntime().Get(MaterialPin, Query);
				}

				if (ChannelComputes.Num() > 3)
				{
					VOXEL_MESSAGE(Error, "{0}: More than 3 channels being debugged at once: {1}", this, NodeRefs);
					ChannelComputes.SetNum(3);
					ChannelComputes.SetNum(3);
				}

				TVoxelArray<TValue<FVoxelMaterialParameter>> Parameters;
				for (int32 Index = 0; Index < ChannelComputes.Num(); Index++)
				{
					const TSharedPtr<const TVoxelComputeValue<FVoxelFloatBuffer>> Compute = ChannelComputes[Index];

					Parameters.Add(VOXEL_CALL_NODE(FVoxelNode_MakeFloatDetailTextureParameter, ParameterPin, Query)
					{
						VOXEL_CALL_NODE_BIND(FloatPin, Compute)
						{
							return (*Compute)(Query);
						};
						VOXEL_CALL_NODE_BIND(TexturePin, Index)
						{
							return
								MakeVoxelTask()
								.RunOnGameThread()
								.Execute<FVoxelFloatDetailTextureRef>([=]() -> FVoxelFloatDetailTextureRef
								{
									if (!GetDefault<UVoxelSettings>()->MarchingCubeDebugDetailTextures.IsValidIndex(Index))
									{
										return {};
									}

									UVoxelFloatDetailTexture* Texture = GetDefault<UVoxelSettings>()->MarchingCubeDebugDetailTextures[Index].LoadSynchronous();
									if (!Texture)
									{
										return {};
									}

									return FVoxelFloatDetailTextureRef{ GVoxelDetailTextureManager->FindOrAdd_GameThread(*Texture) };
								});
						};
					});
				}

				return
					MakeVoxelTask()
					.RunOnGameThread()
					.Dependencies(Parameters)
					.Execute<FVoxelMaterial>([=]
					{
						FVoxelMaterial Result;
						Result.ParentMaterial = FVoxelMaterialRef::Make(GetDefault<UVoxelSettings>()->MarchingCubeDebugMaterial.LoadSynchronous());
						for (const TValue<FVoxelMaterialParameter>& Parameter : Parameters)
						{
							Result.Parameters.Add(Parameter.GetShared_CheckCompleted());
						}
						return Result;
					});
			};

			const TValue<FVoxelMaterial> Material =
				MakeVoxelTask()
				.Dependencies(MaterialTemplate, FutureMaterialMaterialParameter)
				.Execute<FVoxelMaterial>([=]
				{
					TVoxelArray<TValue<FVoxelMaterialParameter>> Parameters;
#if 0 // TODO
					const TSharedRef<const FVoxelSurface> Surface = FutureSurface.GetShared_CheckCompleted();
					const TVoxelMap<FName, FVoxelPinType> InnerTypes = Surface->GetInnerTypes();

					for (const auto& It : Surface->GetDetailTextures())
					{
						const FName Name = It.Key;
						const FVoxelSurface::FDetailTextureInfo& DetailTextureInfo = It.Value;
						const FVoxelPinType InnerType = InnerTypes.FindRef(Name);
						const TSharedPtr<FVoxelDetailTexture> DetailTexture = DetailTextureInfo.WeakDetailTexture.Pin();

						if (!InnerType.IsValid() ||
							!DetailTexture.IsValid())
						{
							continue;
						}

						const FVoxelPinType ExpectedInnerType = DetailTexture->Class.GetDefaultObject()->GetInnerType();
						if (ExpectedInnerType != InnerType)
						{
							VOXEL_MESSAGE(Error, "{0}: Cannot bind a {1} to an attribute of type {2}, should have type {3}",
								DetailTextureInfo.BindNode,
								DetailTexture->Class->GetName(),
								InnerType.ToString(),
								ExpectedInnerType.ToString());
							continue;
						}

						if (DetailTexture->Class == StaticClassFast<UVoxelFloatDetailTexture>())
						{
							Parameters.Add(VOXEL_CALL_NODE(FVoxelNode_MakeFloatDetailTextureParameter, ParameterPin, Query)
							{
								VOXEL_CALL_NODE_BIND(FloatPin, Surface, Name)
								{
									return Surface->GetTyped<FVoxelFloatBuffer>(Name, Query);
								};
								VOXEL_CALL_NODE_BIND(TexturePin, DetailTextureInfo)
								{
									return FVoxelFloatDetailTextureRef{ DetailTextureInfo.WeakDetailTexture };
								};
							});
						}
						else if (DetailTexture->Class == StaticClassFast<UVoxelColorDetailTexture>())
						{
							Parameters.Add(VOXEL_CALL_NODE(FVoxelNode_MakeColorDetailTextureParameter, ParameterPin, Query)
							{
								VOXEL_CALL_NODE_BIND(ColorPin, Surface, Name)
								{
									return Surface->GetTyped<FVoxelLinearColorBuffer>(Name, Query);
								};
								VOXEL_CALL_NODE_BIND(TexturePin, DetailTextureInfo)
								{
									return FVoxelColorDetailTextureRef{ DetailTextureInfo.WeakDetailTexture };
								};
							});
						}
						else if (DetailTexture->Class == StaticClassFast<UVoxelMaterialDetailTexture>())
						{
							Parameters.Add(VOXEL_CALL_NODE(FVoxelNode_MakeMaterialDetailTextureParameter, ParameterPin, Query)
							{
								VOXEL_CALL_NODE_BIND(MaterialPin, Surface, Name)
								{
									return Surface->GetTyped<FVoxelMaterialBlendingBuffer>(Name, Query);
								};
								VOXEL_CALL_NODE_BIND(TexturePin, DetailTextureInfo)
								{
									return FVoxelMaterialDetailTextureRef{ DetailTextureInfo.WeakDetailTexture };
								};
							});
						}
						else if (DetailTexture->Class == StaticClassFast<UVoxelNormalDetailTexture>())
						{
							Parameters.Add(VOXEL_CALL_NODE(FVoxelNode_MakeNormalDetailTextureParameter, ParameterPin, Query)
							{
								VOXEL_CALL_NODE_BIND(NormalPin, Surface, Name)
								{
									return Surface->GetTyped<FVoxelVectorBuffer>(Name, Query);
								};
								VOXEL_CALL_NODE_BIND(TexturePin, DetailTextureInfo)
								{
									return FVoxelNormalDetailTextureRef{ DetailTextureInfo.WeakDetailTexture };
								};
							});
						}
						else
						{
							ensure(false);
						}
					}
#endif

					return
						MakeVoxelTask()
						.Dependencies(Parameters)
						.Execute<FVoxelMaterial>([=]
						{
							FVoxelMaterial Result = MaterialTemplate.Get_CheckCompleted();

							if (FutureMaterialMaterialParameter.Get_CheckCompleted().IsA<FVoxelDetailTextureParameter>())
							{
								// Only add if valid
								Result.Parameters.Add(FutureMaterialMaterialParameter.GetShared_CheckCompleted());
							}

							for (const TValue<FVoxelMaterialParameter>& Parameter : Parameters)
							{
								Result.Parameters.Add(Parameter.GetShared_CheckCompleted());
							}
							return Result;
						});
				});

			if (LOD == 0)
			{
				return Material;
			}

			const TValue<FVoxelMaterialParameter> NormalMaterialParameter = VOXEL_CALL_NODE(FVoxelNode_MakeNormalDetailTextureParameter, ParameterPin, Query)
			{
				CalleeNode.bIsMain = true;

				VOXEL_CALL_NODE_BIND(NormalPin)
				{
					return VOXEL_CALL_NODE(FVoxelNode_GetGradient, GradientPin, Query)
					{
						VOXEL_CALL_NODE_BIND(ValuePin)
						{
							const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
							Parameters->Add<FVoxelSurfaceQueryParameter>().ComputeDistance();
							const TValue<FVoxelSurface> Surface = GetSurface(Query.MakeNewQuery(Parameters));

							return VOXEL_ON_COMPLETE(Surface)
							{
								return Surface->GetDistance();
							};
						};
					};
				};
				VOXEL_CALL_NODE_BIND(TexturePin)
				{
					return GetNodeRuntime().Get(NormalDetailTexturePin, Query);
				};
			};

			return VOXEL_ON_COMPLETE(LOD, Material, NormalMaterialParameter)
			{
				if (!NormalMaterialParameter->IsA<FVoxelDetailTextureParameter>() ||
					!NormalMaterialParameter->AsChecked<FVoxelDetailTextureParameter>().DetailTexture)
				{
					VOXEL_MESSAGE(Error, "{0}: Normal detail texture is null", this);
				}

				FVoxelMaterial Result = *Material;
				Result.Parameters.Add(NormalMaterialParameter);
				return Result;
			};
		};

		VOXEL_CALL_NODE_BIND(GenerateDistanceFieldPin)
		{
			return GetNodeRuntime().Get(GenerateDistanceFieldsPin, Query);
		};

		VOXEL_CALL_NODE_BIND(DistanceFieldBiasPin)
		{
			return GetNodeRuntime().Get(DistanceFieldBiasPin, Query);
		};

		VOXEL_CALL_NODE_BIND(DistanceFieldSelfShadowBiasPin)
		{
			return GetNodeRuntime().Get(DistanceFieldSelfShadowBiasPin, Query);
		};
	};

	const TValue<FVoxelMarchingCubeMeshComponentSettings> MeshSettings = GetNodeRuntime().Get(MeshSettingsPin, Query);
	const TValue<FBodyInstance> BodyInstance =
		Query.GetInfo(EVoxelQueryInfo::Query).IsGameWorld()
		? GetNodeRuntime().Get(BodyInstancePin, Query)
		// Always collide in editor for sculpting & drag/drop
		: FBodyInstance();

	return
		MakeVoxelTask("CreateCollider")
		.Dependencies(Mesh, MeshSettings, BodyInstance)
		.Execute<FVoxelMarchingCubeExecNodeMesh>([=]() -> TValue<FVoxelMarchingCubeExecNodeMesh>
		{
			const TSharedRef<FVoxelMarchingCubeExecNodeMesh> Result = MakeVoxelShared<FVoxelMarchingCubeExecNodeMesh>();
			if (!Mesh.Get_CheckCompleted().Mesh)
			{
				return Result;
			}
			Result->Mesh = Mesh.Get_CheckCompleted().Mesh;
			Result->MeshSettings = MeshSettings.GetShared_CheckCompleted();

			if (Query.GetInfo(EVoxelQueryInfo::Query).FindParameter<FVoxelRuntimeParameter_DisableCollision>() ||
				BodyInstance.Get_CheckCompleted().GetCollisionEnabled(false) == ECollisionEnabled::NoCollision)
			{
				return Result;
			}

			const TValue<FVoxelMarchingCubeColliderWrapper> ColliderWrapper = VOXEL_CALL_NODE(FVoxelNode_CreateMarchingCubeCollider, ColliderPin, Query)
			{
				VOXEL_CALL_NODE_BIND(SurfacePin, MarchingCubeSurface)
				{
					return MarchingCubeSurface;
				};

				VOXEL_CALL_NODE_BIND(PhysicalMaterialPin)
				{
					return GetNodeRuntime().Get(PhysicalMaterialPin, Query);
				};
			};

			return
				MakeVoxelTask()
				.Dependencies(ColliderWrapper)
				.Execute<FVoxelMarchingCubeExecNodeMesh>([=]
				{
					Result->Collider = ColliderWrapper.Get_CheckCompleted().Collider;
					Result->BodyInstance = BodyInstance.GetShared_CheckCompleted();

					// If mesh isn't empty collider shouldn't be either
					ensure(Result->Collider);
					return Result;
				});
		});
}

TVoxelFutureValue<FVoxelSurface> FVoxelExecNode_OutputMarchingCubeMesh::GetSurface(const FVoxelQuery& Query) const
{
	TSharedPtr<const FVoxelDebugQueryParameter> DebugParameter = Query.GetParameters().FindShared<FVoxelDebugQueryParameter>();
	if (!DebugParameter)
	{
		DebugParameter = MakeVoxelShared<FVoxelDebugQueryParameter>();
	}

	const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
	Parameters->Add(DebugParameter.ToSharedRef());
	const TValue<FVoxelSurface> Surface = GetNodeRuntime().Get(SurfacePin, Query.EnterScope(*this).MakeNewQuery(Parameters));

	return
		MakeVoxelTask()
		.Dependency(Surface)
		.Execute<FVoxelSurface>([=]
		{
			VOXEL_SCOPE_LOCK(DebugParameter->CriticalSection);

			if (DebugParameter->Entries_RequiresLock.Num() == 0)
			{
				return Surface;
			}

			TVoxelArray<FVoxelGraphNodeRef> NodeRefs;
			TVoxelArray<TSharedPtr<const TVoxelComputeValue<FVoxelSurface>>> Computes;
			for (const auto& It : DebugParameter->Entries_RequiresLock)
			{
				const FVoxelDebugQueryParameter::FEntry& Entry = It.Value;
				if (!Entry.Type.Is<FVoxelSurface>())
				{
					continue;
				}

				NodeRefs.Add(Entry.NodeRef);
				Computes.Add(ReinterpretCastSharedPtr<const TVoxelComputeValue<FVoxelSurface>>(Entry.Compute));
			}

			if (Computes.Num() == 0)
			{
				return Surface;
			}

			if (Computes.Num() > 1)
			{
				VOXEL_MESSAGE(Error, "{0}: More than 1 surface being debugged at once: {1}", this, NodeRefs);
			}

			return (*Computes[0])(Query);
		});
}

TVoxelUniquePtr<FVoxelExecNodeRuntime> FVoxelExecNode_OutputMarchingCubeMesh::CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const
{
	if (!FApp::CanEverRender())
	{
		// Never create on server
		return nullptr;
	}

	return MakeVoxelUnique<FVoxelExecNodeRuntime_OutputMarchingCubeMesh>(SharedThis);
}