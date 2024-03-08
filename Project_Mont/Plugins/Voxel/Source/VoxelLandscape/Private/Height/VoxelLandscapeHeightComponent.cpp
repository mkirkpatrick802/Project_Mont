// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Height/VoxelLandscapeHeightComponent.h"
#include "Height/VoxelLandscapeHeightMesh.h"
#include "Height/VoxelLandscapeHeightSceneProxy.h"
#include "Materials/MaterialInterface.h"

UVoxelLandscapeHeightComponent::UVoxelLandscapeHeightComponent()
{
	CastShadow = true;
	bUseAsOccluder = true;
}

void UVoxelLandscapeHeightComponent::SetMesh(const TSharedPtr<FVoxelLandscapeHeightMesh>& NewMesh)
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

FPrimitiveSceneProxy* UVoxelLandscapeHeightComponent::CreateSceneProxy()
{
	VOXEL_FUNCTION_COUNTER();

	if (!Mesh)
	{
		return nullptr;
	}

	return new FVoxelLandscapeHeightSceneProxy(*this);
}

int32 UVoxelLandscapeHeightComponent::GetNumMaterials() const
{
	return 1;
}

UMaterialInterface* UVoxelLandscapeHeightComponent::GetMaterial(int32 ElementIndex) const
{
	if (!Mesh)
	{
		return nullptr;
	}

	return Mesh->Material->GetMaterial();
}

void UVoxelLandscapeHeightComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (!Mesh)
	{
		return;
	}

	OutMaterials.Add(Mesh->Material->GetMaterial());
}

FBoxSphereBounds UVoxelLandscapeHeightComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FVoxelBox LocalBounds = Mesh ? Mesh->Bounds.Extend(BoundsExtension) : FVoxelBox();
	ensure(LocalBounds.IsValid());
	return LocalBounds.TransformBy(LocalToWorld).ToFBox();
}

void UVoxelLandscapeHeightComponent::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	VOXEL_FUNCTION_COUNTER();

	Super::OnComponentDestroyed(bDestroyingHierarchy);

	// Clear memory
	Mesh.Reset();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FMaterialRelevance UVoxelLandscapeHeightComponent::GetMaterialRelevance(const ERHIFeatureLevel::Type InFeatureLevel) const
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