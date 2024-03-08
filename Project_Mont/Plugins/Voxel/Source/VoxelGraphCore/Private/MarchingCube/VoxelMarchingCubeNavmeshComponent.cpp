// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MarchingCube/VoxelMarchingCubeNavmeshComponent.h"
#include "MarchingCube/VoxelMarchingCubeNavmesh.h"
#include "AI/NavigationSystemBase.h"
#include "AI/NavigationSystemHelpers.h"

UVoxelMarchingCubeNavmeshComponent::UVoxelMarchingCubeNavmeshComponent()
{
	bCanEverAffectNavigation = true;
	bHasCustomNavigableGeometry = EHasCustomNavigableGeometry::EvenIfNotCollidable;
}

void UVoxelMarchingCubeNavmeshComponent::SetNavigationMesh(const TSharedPtr<const FVoxelMarchingCubeNavmesh>& NewNavmesh)
{
	VOXEL_FUNCTION_COUNTER();

	Navmesh = NewNavmesh;

	if (Navmesh)
	{
		Navmesh->UpdateStats();
	}

	UpdateBounds();

	if (IsRegistered() &&
		GetWorld() &&
		GetWorld()->GetNavigationSystem() &&
		FNavigationSystem::WantsComponentChangeNotifies())
	{
		VOXEL_SCOPE_COUNTER("UpdateComponentData");

		bNavigationRelevant = IsNavigationRelevant();
		FNavigationSystem::UpdateComponentData(*this);
	}
}

void UVoxelMarchingCubeNavmeshComponent::ReturnToPool()
{
	SetNavigationMesh(nullptr);
}

bool UVoxelMarchingCubeNavmeshComponent::IsNavigationRelevant() const
{
	return Navmesh.IsValid();
}

bool UVoxelMarchingCubeNavmeshComponent::DoCustomNavigableGeometryExport(FNavigableGeometryExport& GeomExport) const
{
	VOXEL_FUNCTION_COUNTER();

	if (Navmesh)
	{
		const TArray<FVector> DoubleVertices(Navmesh->Vertices);

		GeomExport.ExportCustomMesh(
			DoubleVertices.GetData(),
			Navmesh->Vertices.Num(),
			Navmesh->Indices.GetData(),
			Navmesh->Indices.Num(),
			GetComponentTransform());
	}

	return false;
}

FBoxSphereBounds UVoxelMarchingCubeNavmeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FVoxelBox LocalBounds = Navmesh ? Navmesh->LocalBounds : FVoxelBox();
	ensure(LocalBounds.IsValid());
	return LocalBounds.TransformBy(LocalToWorld).ToFBox();
}

void UVoxelMarchingCubeNavmeshComponent::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	VOXEL_FUNCTION_COUNTER();

	Super::OnComponentDestroyed(bDestroyingHierarchy);

	// Clear memory
	Navmesh.Reset();
}