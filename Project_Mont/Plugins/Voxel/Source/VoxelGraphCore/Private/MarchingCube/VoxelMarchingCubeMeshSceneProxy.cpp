// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MarchingCube/VoxelMarchingCubeMeshSceneProxy.h"
#include "MarchingCube/VoxelMarchingCubeMesh.h"
#include "MarchingCube/VoxelMarchingCubeMeshComponent.h"
#include "MeshBatch.h"
#include "Engine/Engine.h"
#include "SceneInterface.h"
#include "SceneManagement.h"
#include "Materials/Material.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, bool, GVoxelShowMeshSections, false,
	"voxel.mesh.ShowMeshSections",
	"If true, will assign a unique color to each mesh section");

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, bool, GVoxelDisableStaticPath, false,
	"voxel.mesh.DisableStaticPath",
	"");

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, bool, GVoxelDisableOcclusionCulling, false,
	"voxel.mesh.DisableOcclusionCulling",
	"");

FVoxelMarchingCubeMeshSceneProxy::FVoxelMarchingCubeMeshSceneProxy(const UVoxelMarchingCubeMeshComponent& Component)
	: FPrimitiveSceneProxy(&Component)
	, MaterialRelevance(Component.GetMaterialRelevance(GetScene().GetFeatureLevel()))
	, Mesh(Component.Mesh.ToSharedRef())
	, bOnlyDrawIfSelected(Component.bOnlyDrawIfSelected)
	, OwnerForSelectionCheck(
		Component.GetOwner() && Component.GetOwner()->GetRootSelectionParent()
		? Component.GetOwner()->GetRootSelectionParent()
		: Component.GetOwner())
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

void FVoxelMarchingCubeMeshSceneProxy::CreateRenderThreadResources(UE_504_ONLY(FRHICommandListBase& RHICmdList))
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInRenderingThread());

	Mesh->InitializeIfNeeded_RenderThread(
		UE_504_SWITCH(FRHICommandListExecutor::GetImmediateCommandList(), RHICmdList),
		GetScene().GetFeatureLevel());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMarchingCubeMeshSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<FVoxelMaterialRef> Material = Mesh->GetMaterial();

	// Make sure to keep a ref to the material
	// Otherwise, if the Mesh updates it, the cached static mesh commands might have a dangling ref and crash
	MaterialsToKeepAlive.Enqueue(Material);

	const UMaterialInterface* MaterialObject = Material->GetMaterial();
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

	if (!GVoxelDisableStaticPath)
	{
		PDI->DrawMesh(MeshBatch, FLT_MAX);
	}

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

void FVoxelMarchingCubeMeshSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, const uint32 VisibilityMap, FMeshElementCollector& Collector) const
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

	const TSharedRef<FVoxelMaterialRef> Material = Mesh->GetMaterial();
	const UMaterialInterface* MaterialObject = Material->GetMaterial();
	if (!MaterialObject)
	{
		// Will happen in force delete
		return;
	}

	const FMaterialRenderProxy* MaterialProxy = MaterialObject->GetRenderProxy();

	if (GVoxelShowMeshSections)
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

FPrimitiveViewRelevance FVoxelMarchingCubeMeshSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	const FEngineShowFlags& EngineShowFlags = View->Family->EngineShowFlags;

	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);

#if WITH_EDITOR
	if (bOnlyDrawIfSelected)
	{
		Result.bDrawRelevance =
			!EngineShowFlags.Game &&
			FVoxelUtilities::IsActorSelected_AnyThread(OwnerForSelectionCheck);
	}
#endif

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

bool FVoxelMarchingCubeMeshSceneProxy::CanBeOccluded() const
{
	return !GVoxelDisableOcclusionCulling && !MaterialRelevance.bDisableDepthTest;
}

uint32 FVoxelMarchingCubeMeshSceneProxy::GetMemoryFootprint() const
{
	return sizeof(*this) + GetAllocatedSize();
}

SIZE_T FVoxelMarchingCubeMeshSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMarchingCubeMeshSceneProxy::GetDistanceFieldAtlasData(const FDistanceFieldVolumeData*& OutDistanceFieldData, float& SelfShadowBias) const
{
	OutDistanceFieldData = Mesh->DistanceFieldVolumeData.Get();
	SelfShadowBias = Mesh->SelfShadowBias;
}

void FVoxelMarchingCubeMeshSceneProxy::GetDistanceFieldInstanceData(TArray<FRenderTransform>& ObjectLocalToWorldTransforms) const
{
	if (FVoxelMarchingCubeMeshSceneProxy::HasDistanceFieldRepresentation())
	{
		ObjectLocalToWorldTransforms.Add(FRenderTransform::Identity);
	}
}

const FCardRepresentationData* FVoxelMarchingCubeMeshSceneProxy::GetMeshCardRepresentation() const
{
	return Mesh->CardRepresentationData.Get();
}

bool FVoxelMarchingCubeMeshSceneProxy::HasDistanceFieldRepresentation() const
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

bool FVoxelMarchingCubeMeshSceneProxy::HasDynamicIndirectShadowCasterRepresentation() const
{
	return bCastsDynamicIndirectShadow && FVoxelMarchingCubeMeshSceneProxy::HasDistanceFieldRepresentation();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelMarchingCubeMeshSceneProxy::ShouldUseStaticPath(const FSceneViewFamily& ViewFamily) const
{
	return
		!IsRichView(ViewFamily) &&
		!GVoxelDisableStaticPath &&
		!GVoxelShowMeshSections;
}

bool FVoxelMarchingCubeMeshSceneProxy::DrawMesh(
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
	ensure(Mesh->NumIndicesToRender % 3 == 0);

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
	BatchElement.NumPrimitives = Mesh->NumIndicesToRender / 3;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = Mesh->NumVerticesToRender - 1;

#if UE_ENABLE_DEBUG_DRAWING
	MeshBatch.VisualizeLODIndex = Mesh->LOD % GEngine->LODColorationColors.Num();
#endif

	return true;
}