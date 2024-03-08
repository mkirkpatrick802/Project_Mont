// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MarchingCube/VoxelMarchingCubeCollisionComponent.h"
#include "MarchingCube/VoxelMarchingCubeCollider.h"
#include "MarchingCube/VoxelMarchingCubeCollisionSceneProxy.h"
#include "Materials/Material.h"
#include "PhysicsEngine/BodySetup.h"
#include "Chaos/TriangleMeshImplicitObject.h"

void UVoxelMarchingCubeCollisionComponent::SetBodyInstance(const FBodyInstance& NewBodyInstance)
{
	FVoxelUtilities::CopyBodyInstance(
		BodyInstance,
		NewBodyInstance);
}

void UVoxelMarchingCubeCollisionComponent::SetCollider(const TSharedPtr<const FVoxelMarchingCubeCollider>& NewCollider)
{
	VOXEL_FUNCTION_COUNTER();

	Collider = NewCollider;

	if (Collider)
	{
		Collider->UpdateStats();

		static TMap<TWeakObjectPtr<UPhysicalMaterial>, TWeakObjectPtr<UMaterial>> MaterialCache;
		for (auto It = MaterialCache.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid() ||
				!It.Value().IsValid())
			{
				It.RemoveCurrent();
			}
		}

		for (const TWeakObjectPtr<UPhysicalMaterial>& PhysicalMaterial : Collider->PhysicalMaterials)
		{
			TWeakObjectPtr<UMaterial>& Material = MaterialCache.FindOrAdd(PhysicalMaterial);
			if (!Material.IsValid())
			{
				Material = NewObject<UMaterial>();
				Material->PhysMaterial = PhysicalMaterial.Get();
			}
			Materials.Add(Material.Get());
		}
	}
	else
	{
		Materials.Reset();
	}

	if (BodySetup)
	{
		BodySetup->ClearPhysicsMeshes();
	}
	else
	{
		BodySetup = NewObject<UBodySetup>(this);
		BodySetup->bGenerateMirroredCollision = false;
		BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
	}

	if (Collider)
	{
		ensure(BodySetup->UE_504_SWITCH(ChaosTriMeshes, TriMeshGeometries).Num() == 0);

		// Copied from UBodySetup::FinishCreatingPhysicsMeshes_Chaos
#if TRACK_CHAOS_GEOMETRY
		Collider->TriangleMesh->Track(MakeSerializable(Collider->TriangleMesh UE_504_SWITCH(.ToSharedPtr(),)), "Voxel Mesh");
#endif
		ensure(!Collider->TriangleMesh->GetDoCollide());

		BodySetup->UE_504_SWITCH(ChaosTriMeshes, TriMeshGeometries).Add(Collider->TriangleMesh);
		BodySetup->bCreatedPhysicsMeshes = true;
	}

	BodyInstance.OwnerComponent = this;
	BodyInstance.UpdatePhysicalMaterials();

	RecreatePhysicsState();
	MarkRenderStateDirty();
}

void UVoxelMarchingCubeCollisionComponent::ReturnToPool()
{
	VOXEL_FUNCTION_COUNTER();

	SetBodyInstance(FBodyInstance());
	SetCollider(nullptr);
}

bool UVoxelMarchingCubeCollisionComponent::ShouldCreatePhysicsState() const
{
	return Collider.IsValid();
}

FBoxSphereBounds UVoxelMarchingCubeCollisionComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FBox LocalBounds = Collider.IsValid() ? Collider->LocalBounds.ToFBox() : FBox(FVector::ZeroVector, FVector::ZeroVector);
	ensure(!LocalBounds.Min.ContainsNaN());
	ensure(!LocalBounds.Max.ContainsNaN());
	return LocalBounds.TransformBy(LocalToWorld);
}

void UVoxelMarchingCubeCollisionComponent::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	VOXEL_FUNCTION_COUNTER();

	Super::OnComponentDestroyed(bDestroyingHierarchy);

	// Clear memory
	Collider.Reset();

	if (BodySetup)
	{
		BodySetup->ClearPhysicsMeshes();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FPrimitiveSceneProxy* UVoxelMarchingCubeCollisionComponent::CreateSceneProxy()
{
	if (!GIsEditor ||
		!Collider)
	{
		return nullptr;
	}

	return new FVoxelMarchingCubeCollisionSceneProxy(*this);
}

int32 UVoxelMarchingCubeCollisionComponent::GetNumMaterials() const
{
	return Materials.Num();
}

UMaterialInterface* UVoxelMarchingCubeCollisionComponent::GetMaterial(const int32 ElementIndex) const
{
	if (Materials.Num() == 0)
	{
		return nullptr;
	}

	if (!ensure(Materials.IsValidIndex(ElementIndex)))
	{
		return nullptr;
	}

	return Materials[ElementIndex];
}