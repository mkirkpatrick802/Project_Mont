// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Material/VoxelMaterialDefinitionToolkit.h"
#include "Material/SVoxelMaterialDefinitionParameters.h"
#include "Material/VoxelMaterialDefinitionParameterSelectionCustomization.h"

void FVoxelMaterialDefinitionToolkit::Initialize()
{
	Super::Initialize();

	MaterialLayerParameters =
		SNew(SVoxelMaterialDefinitionParameters)
		.Toolkit(SharedThis(this));
}

TSharedPtr<FTabManager::FLayout> FVoxelMaterialDefinitionToolkit::GetLayout() const
{
	return FTabManager::NewLayout("FVoxelMaterialDefinitionToolkit_Definition_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.3f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.4f)
					->AddTab(DetailsTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->AddTab(ParametersTabId, ETabState::OpenedTab)
				)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.7f)
				->AddTab(ViewportTabId, ETabState::OpenedTab)
			)
		);
}

void FVoxelMaterialDefinitionToolkit::RegisterTabs(const FRegisterTab RegisterTab)
{
	Super::RegisterTabs(RegisterTab);

	RegisterTab(ParametersTabId, INVTEXT("Parameters"), "ClassIcon.BlueprintCore", MaterialLayerParameters);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialDefinitionToolkit::SelectParameter(const FGuid& Guid, const bool bRequestRename, const bool bRefresh)
{
	if (SelectedGuid == Guid &&
		!bRefresh)
	{
		return;
	}
	SelectedGuid = Guid;

	IDetailsView& DetailsView = GetDetailsView();

	if (!CastChecked<UVoxelMaterialDefinition>(Asset)->GuidToMaterialParameter.Contains(Guid))
	{
		MaterialLayerParameters->SelectMember({}, 0, false, bRefresh);

		DetailsView.SetGenericLayoutDetailsDelegate(nullptr);
		DetailsView.ForceRefresh();
		return;
	}

	MaterialLayerParameters->SelectMember(Guid, 1, bRequestRename, bRefresh);

	DetailsView.SetGenericLayoutDetailsDelegate(MakeLambdaDelegate([this, Guid]() -> TSharedRef<IDetailCustomization>
	{
		return MakeVoxelShared<FVoxelMaterialDefinitionParameterSelectionCustomization>(Guid);
	}));
	DetailsView.ForceRefresh();
}