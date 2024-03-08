// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MarchingCube/VoxelMarchingCubeMeshComponent.h"
#include "MarchingCube/VoxelMarchingCubeMesh.h"
#include "MarchingCube/VoxelMarchingCubeMeshSceneProxy.h"
#include "MarchingCube/VoxelMarchingCubeMeshComponentSettings.h"
#include "Materials/MaterialInterface.h"

VOXEL_CONSOLE_SINK(MarchingCubeMeshComponentConsoleSink)
{
	ForEachObjectOfClass<UVoxelMarchingCubeMeshComponent>([&](UVoxelMarchingCubeMeshComponent& Component)
	{
		if (Component.GetMesh())
		{
			Component.MarkRenderStateDirty();
		}
	});
}

UVoxelMarchingCubeMeshComponent::UVoxelMarchingCubeMeshComponent()
{
	CastShadow = true;
	bUseAsOccluder = true;
}

void UVoxelMarchingCubeMeshComponent::SetMesh(const TSharedPtr<FVoxelMarchingCubeMesh>& NewMesh)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	ensureVoxelSlow(IsRegistered());

	Mesh = NewMesh;

	if (Mesh)
	{
		Mesh->Initialize_GameThread(*this);
	}

	MarkRenderStateDirty();

#if WITH_EDITOR
	FVoxelUtilities::EnsureViewportIsUpToDate();
#endif
}

void UVoxelMarchingCubeMeshComponent::ReturnToPool()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	SetMesh(nullptr);
	FVoxelMarchingCubeMeshComponentSettings().ApplyToComponent(*this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FPrimitiveSceneProxy* UVoxelMarchingCubeMeshComponent::CreateSceneProxy()
{
	VOXEL_FUNCTION_COUNTER();

	if (!Mesh)
	{
		return nullptr;
	}

	return new FVoxelMarchingCubeMeshSceneProxy(*this);
}

int32 UVoxelMarchingCubeMeshComponent::GetNumMaterials() const
{
	return 1;
}

UMaterialInterface* UVoxelMarchingCubeMeshComponent::GetMaterial(int32 ElementIndex) const
{
	if (!Mesh)
	{
		return nullptr;
	}

	return Mesh->GetMaterial()->GetMaterial();
}

void UVoxelMarchingCubeMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (!Mesh)
	{
		return;
	}

	OutMaterials.Add(Mesh->GetMaterial()->GetMaterial());
}

FBoxSphereBounds UVoxelMarchingCubeMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FVoxelBox LocalBounds = Mesh ? Mesh->GetBounds().Extend(BoundsExtension) : FVoxelBox();
	ensure(LocalBounds.IsValid());
	return LocalBounds.TransformBy(LocalToWorld).ToFBox();
}

void UVoxelMarchingCubeMeshComponent::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	VOXEL_FUNCTION_COUNTER();

	Super::OnComponentDestroyed(bDestroyingHierarchy);

	// Clear memory
	Mesh.Reset();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FMaterialRelevance UVoxelMarchingCubeMeshComponent::GetMaterialRelevance(const ERHIFeatureLevel::Type InFeatureLevel) const
{
	if (!Mesh)
	{
		return {};
	}

	const UMaterialInterface* Material = Mesh->GetMaterial()->GetMaterial();
	if (!Material)
	{
		return {};
	}

	return Material->GetRelevance_Concurrent(InFeatureLevel);
}