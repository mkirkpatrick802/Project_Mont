// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelGraphPinParameter.h"
#include "VoxelParameterView.h"
#include "VoxelGraphParametersView.h"
#include "SVoxelGraphParameterComboBox.h"
#include "K2Node_VoxelGraphParameterBase.h"

void SVoxelGraphPinParameter::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	if (const UEdGraphNode* Node = InGraphPinObj->GetOwningNode())
	{
		if (UEdGraphPin* AssetPin = Node->FindPin(UK2Node_VoxelGraphParameterBase::AssetPinName))
		{
			ensure(!AssetPin->DefaultObject || AssetPin->DefaultObject->Implements<UVoxelParameterOverridesObjectOwner>());

			OverridesOwner = MakeWeakInterfacePtr<IVoxelParameterOverridesObjectOwner>(AssetPin->DefaultObject);
			AssetPinReference = AssetPin;
		}
	}

	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

void SVoxelGraphPinParameter::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphPin::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bGraphDataInvalid)
	{
		AssetPinReference = {};
		OverridesOwner = nullptr;
		return;
	}

	if (const UEdGraphPin* Pin = AssetPinReference.Get())
	{
		if (OverridesOwner.GetObject() == Pin->DefaultObject)
		{
			return;
		}

		if (Pin->DefaultObject &&
			!Pin->DefaultObject->Implements<UVoxelParameterOverridesObjectOwner>())
		{
			return;
		}

		OverridesOwner = TWeakInterfacePtr<IVoxelParameterOverridesObjectOwner>(Pin->DefaultObject);
	}
}

TSharedRef<SWidget>	SVoxelGraphPinParameter::GetDefaultValueWidget()
{
	return
		SNew(SBox)
		.MinDesiredWidth(18)
		.MaxDesiredWidth(400)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(TextContainer, SBox)
				.Visibility_Lambda([this]
				{
					if (!OverridesOwner.IsValid())
					{
						return GetDefaultValueVisibility();
					}
					return EVisibility::Collapsed;
				})
				[
					SNew(SEditableTextBox)
					.Style(FAppStyle::Get(), "Graph.EditableTextBox")
					.Text_Lambda([this]
					{
						return FText::FromString(GraphPinObj->DefaultValue);
					})
					.SelectAllTextWhenFocused(true)
					.IsReadOnly_Lambda([this]() -> bool
					{
						return GraphPinObj->bDefaultValueIsReadOnly;
					})
					.OnTextCommitted_Lambda([this](const FText& NewValue, ETextCommit::Type)
					{
						if (!ensure(!GraphPinObj->IsPendingKill()))
						{
							return;
						}

						FString NewName = NewValue.ToString();
						if (NewName.IsEmpty())
						{
							NewName = "None";
						}

						const FVoxelTransaction Transaction(GraphPinObj, "Change Parameter Pin Value");
						GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, *NewName);
					})
					.ForegroundColor(FSlateColor::UseForeground())
				]
			]
			+ SOverlay::Slot()
			[
				SAssignNew(ParameterSelectorContainer, SBox)
				.Visibility_Lambda([this]
				{
					if (OverridesOwner.IsValid())
					{
						return GetDefaultValueVisibility();
					}
					return EVisibility::Collapsed;
				})
				[
					SAssignNew(ParameterComboBox, SVoxelGraphParameterComboBox)
					.Items_Lambda(MakeWeakPtrLambda(this, [this]() -> TArray<FVoxelSelectorItem>
					{
						if (!OverridesOwner.IsValid())
						{
							return {};
						}

						const TSharedPtr<FVoxelGraphParametersView> ParametersView = OverridesOwner->GetParametersView();
						if (!ParametersView)
						{
							return {};
						}

						TArray<FVoxelSelectorItem> Items;
						for (const FVoxelParameterView* ParameterView : ParametersView->GetChildren())
						{
							const FVoxelParameter Parameter = ParameterView->GetParameter();
							Items.Add(FVoxelSelectorItem(ParameterView->GetGuid(), Parameter.Name, Parameter.Type, Parameter.Category));
						}
						return Items;
					}))
					.CurrentItem_Lambda(MakeWeakPtrLambda(this, [this]() -> FVoxelSelectorItem
					{
						const FVoxelGraphBlueprintParameter& CurrentParameter = Cast<UK2Node_VoxelGraphParameterBase>(GraphPinObj->GetOwningNode())->CachedParameter;
						return FVoxelSelectorItem(CurrentParameter.Guid, CurrentParameter.Name, CurrentParameter.Type);
					}))
					.IsValidItem_Lambda(MakeWeakPtrLambda(this, [this]
					{
						const FVoxelGraphBlueprintParameter& CurrentParameter = Cast<UK2Node_VoxelGraphParameterBase>(GraphPinObj->GetOwningNode())->CachedParameter;
						return CurrentParameter.bIsValid;
					}))
					.OnItemChanged(MakeWeakPtrDelegate(this, [this](const FVoxelSelectorItem& NewItem)
					{
						if (!ensure(!GraphPinObj->IsPendingKill()) ||
							!GraphPinObj->GetOwningNode())
						{
							return;
						}

						const FVoxelTransaction Transaction(GraphPinObj, "Change Parameter Pin Value");
						GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, NewItem.Guid.ToString());
					}))
				]
			]
		];
}