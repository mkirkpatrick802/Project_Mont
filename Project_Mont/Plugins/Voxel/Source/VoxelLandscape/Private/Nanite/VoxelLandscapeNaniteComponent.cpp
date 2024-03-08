// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nanite/VoxelLandscapeNaniteComponent.h"
#include "Nanite/VoxelLandscapeNaniteMesh.h"

void UVoxelLandscapeNaniteComponent::SetMesh(const TSharedPtr<FVoxelLandscapeNaniteMesh>& NewMesh)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(Mesh != NewMesh);
	Mesh = NewMesh;

	if (!Mesh)
	{
		SetStaticMesh(nullptr);
		return;
	}

	SetStaticMesh(Mesh->GetStaticMesh());
}

void UVoxelLandscapeNaniteComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	VOXEL_FUNCTION_COUNTER();

	Super::OnComponentDestroyed(bDestroyingHierarchy);

	// Clear memory
	Mesh.Reset();
}