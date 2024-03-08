// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Material/VoxelMaterialDefinitionInstanceToolkit.h"

TSharedPtr<FTabManager::FLayout> FVoxelMaterialDefinitionInstanceToolkit::GetLayout() const
{
	return FTabManager::NewLayout("FVoxelMaterialDefinitionInstanceToolkit_Instance_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.4f)
				->AddTab(DetailsTabId, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.7f)
				->AddTab(ViewportTabId, ETabState::OpenedTab)
			)
		);
}