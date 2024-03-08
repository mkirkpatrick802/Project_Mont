// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Volume/VoxelLandscapeVolumeComponent.h"
#include "Volume/VoxelLandscapeVolumeMesh.h"
#include "Volume/VoxelLandscapeVolumeSceneProxy.h"
#include "Materials/MaterialInterface.h"

UVoxelLandscapeVolumeComponent::UVoxelLandscapeVolumeComponent()
{
	CastShadow = true;
	bUseAsOccluder = true;
}

void UVoxelLandscapeVolumeComponent::SetMesh(const TSharedPtr<FVoxelLandscapeVolumeMesh>& NewMesh)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	ensureVoxelSlow(IsRegistered());

	ensure(Mesh != NewMesh);
	Mesh = NewMesh;

	MarkRenderStateDirty();

#if WITH_EDITOR
	FVoxelUtilities::EnsureViewportIsUpToDate();
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FPrimitiveSceneProxy* UVoxelLandscapeVolumeComponent::CreateSceneProxy()
{
	VOXEL_FUNCTION_COUNTER();

	if (!Mesh)
	{
		return nullptr;
	}

	return new FVoxelLandscapeVolumeSceneProxy(*this);
}

int32 UVoxelLandscapeVolumeComponent::GetNumMaterials() const
{
	return 1;
}

UMaterialInterface* UVoxelLandscapeVolumeComponent::GetMaterial(int32 ElementIndex) const
{
	if (!Mesh)
	{
		return nullptr;
	}

	return Mesh->Material->GetMaterial();
}

void UVoxelLandscapeVolumeComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (!Mesh)
	{
		return;
	}

	OutMaterials.Add(Mesh->Material->GetMaterial());
}

FBoxSphereBounds UVoxelLandscapeVolumeComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FVoxelBox LocalBounds = Mesh ? Mesh->Bounds.Extend(BoundsExtension) : FVoxelBox();
	ensure(LocalBounds.IsValid());
	return LocalBounds.TransformBy(LocalToWorld).ToFBox();
}

void UVoxelLandscapeVolumeComponent::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	VOXEL_FUNCTION_COUNTER();

	Super::OnComponentDestroyed(bDestroyingHierarchy);

	// Clear memory
	Mesh.Reset();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FMaterialRelevance UVoxelLandscapeVolumeComponent::GetMaterialRelevance(const ERHIFeatureLevel::Type InFeatureLevel) const
{
	if (!Mesh)
	{
		return {};
	}

	const UMaterialInterface* Material = Mesh->Material->GetMaterial();
	if (!Material)
	{
		return {};
	}

	return Material->GetRelevance_Concurrent(InFeatureLevel);
}