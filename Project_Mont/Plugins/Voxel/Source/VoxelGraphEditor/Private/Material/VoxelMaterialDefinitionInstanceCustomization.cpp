// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelCategoryBuilder.h"
#include "Material/VoxelMaterialDefinition.h"
#include "Material/VoxelMaterialDefinitionInstance.h"
#include "Material/VoxelMaterialDefinitionParameterDetails.h"

VOXEL_CUSTOMIZE_CLASS(UVoxelMaterialDefinitionInstance)(IDetailLayoutBuilder& DetailLayout)
{
	UVoxelMaterialDefinitionInstance* MaterialInstance = GetUniqueObjectBeingCustomized(DetailLayout);
	if (!ensure(MaterialInstance))
	{
		return;
	}

	UVoxelMaterialDefinition* Definition = MaterialInstance->GetDefinition();
	if (!Definition)
	{
		return;
	}

	Definition->OnParametersChanged.Add(FVoxelEditorUtilities::MakeRefreshDelegate(this, DetailLayout));

	FVoxelCategoryBuilder CategoryBuilder({});

	for (const auto& It : Definition->GuidToMaterialParameter)
	{
		const TSharedRef<FVoxelMaterialDefinitionParameterDetails> ParameterDetails = MakeVoxelShared<FVoxelMaterialDefinitionParameterDetails>(
			It.Key,
			It.Value,
			FVoxelEditorUtilities::MakeRefreshDelegate(this, DetailLayout),
			MaterialInstance);

		KeepAlive(ParameterDetails);

		CategoryBuilder.AddProperty(
			It.Value.Category,
			MakeWeakPtrLambda(this, [=](const FVoxelDetailInterface& DetailInterface)
			{
				ParameterDetails->MakeRow(DetailInterface);
			}));
	}

	CategoryBuilder.Apply(DetailLayout);
}