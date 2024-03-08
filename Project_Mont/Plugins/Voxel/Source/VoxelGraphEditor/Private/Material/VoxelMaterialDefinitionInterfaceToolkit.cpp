// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Material/VoxelMaterialDefinitionInterfaceToolkit.h"
#include "Material/VoxelMaterialDefinitionInterface.h"
#include "Components/StaticMeshComponent.h"

void FVoxelMaterialDefinitionInterfaceToolkit::PostEditChange(const FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChange(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		UpdatePreview();
	}
}

void FVoxelMaterialDefinitionInterfaceToolkit::SetupPreview()
{
	VOXEL_FUNCTION_COUNTER();

	Super::SetupPreview();

	UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EditorMeshes/EditorSphere.EditorSphere"));
	ensure(StaticMesh);

	StaticMeshComponent = NewObject<UStaticMeshComponent>();
	StaticMeshComponent->SetStaticMesh(StaticMesh);
	GetPreviewScene().AddComponent(StaticMeshComponent.Get(), FTransform::Identity);
}

void FVoxelMaterialDefinitionInterfaceToolkit::UpdatePreview()
{
	VOXEL_FUNCTION_COUNTER();

	Super::UpdatePreview();

	if (!ensure(StaticMeshComponent.IsValid()))
	{
		return;
	}

	StaticMeshComponent->SetMaterial(0, CastChecked<UVoxelMaterialDefinitionInterface>(GetAsset())->GetPreviewMaterial());
}

FRotator FVoxelMaterialDefinitionInterfaceToolkit::GetInitialViewRotation() const
{
	return FRotator(-20.0f, 180 + 45.0f, 0.f);
}