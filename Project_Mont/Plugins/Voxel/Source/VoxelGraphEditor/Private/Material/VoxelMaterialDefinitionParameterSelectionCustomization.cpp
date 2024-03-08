// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMaterialDefinitionParameterSelectionCustomization.h"
#include "Material/VoxelMaterialDefinition.h"

void FVoxelMaterialDefinitionParameterSelectionCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Move Config category below all other categories
	DetailLayout.EditCategory("Config").SetSortOrder(10000);

	ParameterWrapper = FVoxelStructDetailsWrapper::Make<UVoxelMaterialDefinition, FVoxelMaterialDefinitionParameter>(
		DetailLayout,
		[Guid = Guid](const UVoxelMaterialDefinition& MaterialDefinition)
		{
			return MaterialDefinition.GuidToMaterialParameter.Find(Guid);
		},
		[Guid = Guid](UVoxelMaterialDefinition& MaterialDefinition, const FVoxelMaterialDefinitionParameter& NewParameter)
		{
			MaterialDefinition.GuidToMaterialParameter.Add(Guid, NewParameter);
		});

	IDetailCategoryBuilder& ParameterCategory = DetailLayout.EditCategory("Parameter", INVTEXT("Parameter"));

	// Move Parameter category to the top
	ParameterCategory.SetSortOrder(0);
	ParameterWrapper->AddChildrenTo(ParameterCategory);

	const TSharedPtr<IPropertyHandle> GuidToParameterDataHandle = FVoxelEditorUtilities::MakePropertyHandle(
		DetailLayout,
		GET_MEMBER_NAME_STATIC(UVoxelMaterialDefinition, GuidToParameterData));

	if (!ensure(GuidToParameterDataHandle))
	{
		return;
	}

	uint32 ParametersCount = 0;
	GuidToParameterDataHandle->GetNumChildren(ParametersCount);

	TSharedPtr<IPropertyHandle> ParameterDataHandle;
	for (uint32 Index = 0; Index < ParametersCount; Index++)
	{
		const TSharedPtr<IPropertyHandle> ValueHandle = GuidToParameterDataHandle->GetChildHandle(Index);
		if (!ensure(ValueHandle))
		{
			continue;
		}
		const TSharedPtr<IPropertyHandle> KeyHandle = ValueHandle->GetKeyHandle();
		if (!ensure(KeyHandle))
		{
			continue;
		}

		if (FVoxelEditorUtilities::GetStructPropertyValue<FGuid>(KeyHandle) != Guid)
		{
			continue;
		}

		ensure(!ParameterDataHandle);
		ParameterDataHandle = ValueHandle;
	}

	if (!ParameterDataHandle)
	{
		return;
	}

	ParameterDataWrapper = FVoxelInstancedStructDetailsWrapper::Make(ParameterDataHandle.ToSharedRef());
	if (!ParameterDataWrapper)
	{
		return;
	}

	IDetailCategoryBuilder& Builder = DetailLayout.EditCategory("Options", INVTEXT("Options"));
	for (const TSharedPtr<IPropertyHandle>& Handle : ParameterDataWrapper->AddChildStructure())
	{
		Builder.AddProperty(Handle);
	}
}