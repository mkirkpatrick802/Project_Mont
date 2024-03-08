// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Height/VoxelLandscapeHeightSceneProxy.h"
#include "Height/VoxelLandscapeHeightMesh.h"
#include "Height/VoxelLandscapeHeightComponent.h"
#include "Height/VoxelLandscapeHeightVertexFactory.h"
#include "MeshBatch.h"
#include "Engine/Engine.h"
#include "SceneInterface.h"
#include "SceneManagement.h"
#include "Materials/Material.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELLANDSCAPE_API, bool, GVoxelLandscapeShowHeightSections, false,
	"voxel.landscape.ShowHeightSections",
	"");

FVoxelLandscapeHeightSceneProxy::FVoxelLandscapeHeightSceneProxy(const UVoxelLandscapeHeightComponent& Component)
	: FPrimitiveSceneProxy(&Component)
	, MaterialRelevance(Component.GetMaterialRelevance(GetScene().GetFeatureLevel()))
	, Mesh(Component.Mesh.ToSharedRef())
{
	// Mesh can't be deformed, required for VSM caching
	bHasDeformableMesh = false;
	bVisibleInLumenScene = Mesh->CardRepresentationData.IsValid();
	bSupportsDistanceFieldRepresentation = MaterialRelevance.bOpaque && !MaterialRelevance.bUsesSingleLayerWaterMaterial;

	EnableGPUSceneSupportFlags();
	FVoxelRenderUtilities::ResetPreviousLocalToWorld(Component, *this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelLandscapeHeightSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
{
	VOXEL_FUNCTION_COUNTER();

	// Make sure to keep a ref to the material
	// Otherwise, if the Mesh updates it, the cached static mesh commands might have a dangling ref and crash
	MaterialsToKeepAlive.Enqueue(Mesh->Material);

	const UMaterialInterface* MaterialObject = Mesh->Material->GetMaterial();
	if (!MaterialObject)
	{
		// Will happen in force delete
		return;
	}
	const FMaterialRenderProxy* MaterialProxy = MaterialObject->GetRenderProxy();

	FMeshBatch MeshBatch;
	if (!DrawMesh(MeshBatch, MaterialProxy, false))
	{
		return;
	}

	// Else the virtual texture check fails in RuntimeVirtualTextureRender.cpp:338
	// and the static mesh isn't rendered at all
	MeshBatch.LODIndex = 0;

	PDI->DrawMesh(MeshBatch, FLT_MAX);

	if (RuntimeVirtualTextureMaterialTypes.Num() > 0)
	{
		// Runtime virtual texture mesh elements.
		MeshBatch.CastShadow = 0;
		MeshBatch.bUseAsOccluder = 0;
		MeshBatch.bUseForDepthPass = 0;
		MeshBatch.bUseForMaterial = 0;
		MeshBatch.bDitheredLODTransition = 0;
		MeshBatch.bRenderToVirtualTexture = 1;

		for (ERuntimeVirtualTextureMaterialType MaterialType : RuntimeVirtualTextureMaterialTypes)
		{
			MeshBatch.RuntimeVirtualTextureMaterialType = uint32(MaterialType);
			PDI->DrawMesh(MeshBatch, FLT_MAX);
		}
	}
}

void FVoxelLandscapeHeightSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, const uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	VOXEL_FUNCTION_COUNTER();

	const FEngineShowFlags& EngineShowFlags = ViewFamily.EngineShowFlags;

#if UE_ENABLE_DEBUG_DRAWING
	// Render bounds
	{
		VOXEL_SCOPE_COUNTER("Render Bounds");
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				RenderBounds(Collector.GetPDI(ViewIndex), EngineShowFlags, GetBounds(), IsSelected());
			}
		}
	}
#endif

	if (EngineShowFlags.Collision ||
		EngineShowFlags.CollisionPawn ||
		EngineShowFlags.CollisionVisibility)
	{
		return;
	}

	if (ShouldUseStaticPath(ViewFamily))
	{
		return;
	}

	VOXEL_SCOPE_COUNTER("Render Mesh");

	const UMaterialInterface* MaterialObject = Mesh->Material->GetMaterial();
	if (!MaterialObject)
	{
		// Will happen in force delete
		return;
	}

	const FMaterialRenderProxy* MaterialProxy = MaterialObject->GetRenderProxy();

	if (GVoxelLandscapeShowHeightSections)
	{
		const uint32 Hash = FVoxelUtilities::MurmurHash64(uint64(&Mesh.Get()));
		MaterialProxy = FVoxelRenderUtilities::CreateColoredRenderProxy(
			Collector,
			ReinterpretCastRef<const FColor>(Hash));
	}

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (!(VisibilityMap & (1 << ViewIndex)))
		{
			continue;
		}

		FMeshBatch& MeshBatch = Collector.AllocateMesh();
		if (DrawMesh(MeshBatch, MaterialProxy, EngineShowFlags.Wireframe))
		{
			VOXEL_SCOPE_COUNTER("Collector.AddMesh");
			Collector.AddMesh(ViewIndex, MeshBatch);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FPrimitiveViewRelevance FVoxelLandscapeHeightSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	const FEngineShowFlags& EngineShowFlags = View->Family->EngineShowFlags;

	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);

	if (ShouldUseStaticPath(*View->Family))
	{
		Result.bStaticRelevance = true;
		Result.bDynamicRelevance = false;
	}
	else
	{
		Result.bStaticRelevance = false;
		Result.bDynamicRelevance = true;
	}

	if (EngineShowFlags.Bounds)
	{
		Result.bDynamicRelevance = true;
	}

	Result.bRenderInMainPass = ShouldRenderInMainPass();
	Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
	Result.bVelocityRelevance = IsMovable() && Result.bOpaque && Result.bRenderInMainPass;

	MaterialRelevance.SetPrimitiveViewRelevance(Result);

	return Result;
}

bool FVoxelLandscapeHeightSceneProxy::CanBeOccluded() const
{
	return !MaterialRelevance.bDisableDepthTest;
}

uint32 FVoxelLandscapeHeightSceneProxy::GetMemoryFootprint() const
{
	return sizeof(*this) + GetAllocatedSize();
}

SIZE_T FVoxelLandscapeHeightSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelLandscapeHeightSceneProxy::GetDistanceFieldAtlasData(const FDistanceFieldVolumeData*& OutDistanceFieldData, float& SelfShadowBias) const
{
	OutDistanceFieldData = Mesh->DistanceFieldVolumeData.Get();
	SelfShadowBias = Mesh->SelfShadowBias;
}

void FVoxelLandscapeHeightSceneProxy::GetDistanceFieldInstanceData(TArray<FRenderTransform>& ObjectLocalToWorldTransforms) const
{
	if (FVoxelLandscapeHeightSceneProxy::HasDistanceFieldRepresentation())
	{
		ObjectLocalToWorldTransforms.Add(FRenderTransform::Identity);
	}
}

const FCardRepresentationData* FVoxelLandscapeHeightSceneProxy::GetMeshCardRepresentation() const
{
	return Mesh->CardRepresentationData.Get();
}

bool FVoxelLandscapeHeightSceneProxy::HasDistanceFieldRepresentation() const
{
	if (!CastsDynamicShadow() ||
		!AffectsDistanceFieldLighting())
	{
		return false;
	}

	return Mesh->DistanceFieldVolumeData.IsValid();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelLandscapeHeightSceneProxy::HasDynamicIndirectShadowCasterRepresentation() const
{
	return bCastsDynamicIndirectShadow && FVoxelLandscapeHeightSceneProxy::HasDistanceFieldRepresentation();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelLandscapeHeightSceneProxy::ShouldUseStaticPath(const FSceneViewFamily& ViewFamily)
{
	return
		!IsRichView(ViewFamily) &&
		!GVoxelLandscapeShowHeightSections;
}

bool FVoxelLandscapeHeightSceneProxy::DrawMesh(
	FMeshBatch& MeshBatch,
	const FMaterialRenderProxy* MaterialRenderProxy,
	const bool bWireframe) const
{
	VOXEL_FUNCTION_COUNTER();
	check(MaterialRenderProxy);

	if (!ensure(Mesh->IndicesBuffer))
	{
		return false;
	}
	ensure(Mesh->NumIndices % 3 == 0);

	MeshBatch.MaterialRenderProxy = MaterialRenderProxy;
	MeshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
	MeshBatch.DepthPriorityGroup = SDPG_World;
	MeshBatch.bUseWireframeSelectionColoring = IsSelected() && bWireframe; // Else mesh LODs view is messed up when actor is selected
	MeshBatch.bCanApplyViewModeOverrides = true;
	MeshBatch.Type = PT_TriangleList;
	MeshBatch.VertexFactory = Mesh->VertexFactory.Get();

	FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
	BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();
	BatchElement.IndexBuffer = Mesh->IndicesBuffer.Get();
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = Mesh->NumIndices / 3;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = 0;

#if UE_ENABLE_DEBUG_DRAWING
	MeshBatch.VisualizeLODIndex = Mesh->ChunkKey.LOD % GEngine->LODColorationColors.Num();
#endif

	return true;
}