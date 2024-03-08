// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Selection/VoxelGraphSelectionCustomization_Input.h"
#include "VoxelTerminalGraph.h"
#include "Nodes/VoxelGraphNode_Input.h"

void FVoxelGraphSelectionCustomization_Input::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	DetailLayout.HideCategory("Config");
	DetailLayout.HideCategory("Function");

	if (!Guid.IsValid())
	{
		return;
	}

	const TSharedRef<FVoxelStructDetailsWrapper> Wrapper = FVoxelStructDetailsWrapper::Make<UVoxelTerminalGraph, FVoxelGraphInput>(
		DetailLayout,
		[Guid = Guid](const UVoxelTerminalGraph& InTerminalGraph)
		{
			return InTerminalGraph.FindInput(Guid);
		},
		[Guid = Guid](UVoxelTerminalGraph& InTerminalGraph, const FVoxelGraphInput& NewInput)
		{
			InTerminalGraph.UpdateInput(Guid, [&](FVoxelGraphInput& InInput)
			{
				InInput = NewInput;
			});
		});
	KeepAlive(Wrapper);

	Wrapper->InstanceMetadataMap.Add("ShowDefaultValue", !WeakInputNode.IsValid() || !WeakInputNode->bExposeDefaultPin ? "true" : "false");

	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Input", INVTEXT("Input"));
	Wrapper->AddChildrenTo(Category);

	const auto SetExposeDefaultPinMessage = [&](const FString& Message)
	{
		IDetailCategoryBuilder& OptionsCategory = DetailLayout.EditCategory("Options", INVTEXT("Options"));

		// Move Options category below all other categories
		OptionsCategory.SetSortOrder(9999);
		OptionsCategory.InitiallyCollapsed(false);

		OptionsCategory.AddCustomRow(INVTEXT("ExposeDefaultPin"))
		.NameContent()
		[
			SNew(SVoxelDetailText)
			.Text(INVTEXT("Expose Default Pin"))
		]
		.ValueContent()
		[
			SNew(SVoxelDetailText)
			.Text(FText::FromString(Message))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
	};

	if (Type != EType::Node)
	{
		SetExposeDefaultPinMessage("Select an input node to expose default as a pin");
		return;
	}

	if (!ensure(WeakInputNode.IsValid()))
	{
		return;
	}

	const UVoxelTerminalGraph* TerminalGraph = WeakInputNode->GetTypedOuter<UVoxelTerminalGraph>();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	if (WeakInputNode->bIsGraphInput &&
		!TerminalGraph->IsMainTerminalGraph())
	{
		SetExposeDefaultPinMessage("Select a graph input in the main graph to expose default as a pin");
		return;
	}

	IDetailCategoryBuilder& OptionsCategory = DetailLayout.EditCategory("Options", INVTEXT("Options"));

	// Move Options category below all other categories
	OptionsCategory.SetSortOrder(9999);
	OptionsCategory.InitiallyCollapsed(false);

	const TSharedPtr<IPropertyHandle> ExposeDefaultPinHandle = OptionsCategory
		.AddExternalObjectProperty(
			{ WeakInputNode.Get() },
			GET_MEMBER_NAME_STATIC(UVoxelGraphNode_Input, bExposeDefaultPin))
		->GetPropertyHandle();

	if (!ensure(ExposeDefaultPinHandle))
	{
		return;
	}

	ExposeDefaultPinHandle->SetOnPropertyValueChanged(FVoxelEditorUtilities::MakeRefreshDelegate(this, DetailLayout));
}