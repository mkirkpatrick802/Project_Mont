// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelGraphPinGraphOutput.h"
#include "K2Node_QueryVoxelGraphOutput.h"
#include "SVoxelGraphParameterComboBox.h"

void SVoxelGraphPinGraphOutput::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	if (const UEdGraphNode* Node = InGraphPinObj->GetOwningNode())
	{
		if (UEdGraphPin* AssetPin = Node->FindPin(UK2Node_QueryVoxelGraphOutput::GraphPinName))
		{
			ensure(!AssetPin->DefaultObject || AssetPin->DefaultObject->IsA<UVoxelGraph>());

			WeakGraph = Cast<UVoxelGraph>(AssetPin->DefaultObject);
			AssetPinReference = AssetPin;
		}
	}

	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

void SVoxelGraphPinGraphOutput::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphPin::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bGraphDataInvalid)
	{
		AssetPinReference = {};
		WeakGraph = nullptr;
		return;
	}

	if (const UEdGraphPin* Pin = AssetPinReference.Get())
	{
		if (WeakGraph.Get() == Pin->DefaultObject)
		{
			return;
		}

		if (Pin->DefaultObject &&
			!Pin->DefaultObject->IsA<UVoxelGraph>())
		{
			return;
		}

		WeakGraph = Cast<UVoxelGraph>(Pin->DefaultObject);
	}
}

TSharedRef<SWidget>	SVoxelGraphPinGraphOutput::GetDefaultValueWidget()
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
					if (!WeakGraph.IsValid())
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

						const FVoxelTransaction Transaction(GraphPinObj, "Change Graph Output Pin Value");
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
					if (WeakGraph.IsValid())
					{
						return GetDefaultValueVisibility();
					}
					return EVisibility::Collapsed;
				})
				[
					SAssignNew(ParameterComboBox, SVoxelGraphParameterComboBox)
					.Items_Lambda(MakeWeakPtrLambda(this, [this]() -> TArray<FVoxelSelectorItem>
					{
						UVoxelGraph* Graph = WeakGraph.Get();
						if (!Graph)
						{
							return {};
						}

						const UVoxelTerminalGraph& TerminalGraph = Graph->GetMainTerminalGraph();
						TArray<FVoxelSelectorItem> Items;
						for (const FGuid OutputGuid : TerminalGraph.GetOutputs())
						{
							const FVoxelGraphOutput* Output = TerminalGraph.FindOutput(OutputGuid);
							if (!ensure(Output))
							{
								continue;
							}

							Items.Add(FVoxelSelectorItem(OutputGuid, Output->Name, Output->Type, Output->Category));
						}

						return Items;
					}))
					.CurrentItem_Lambda(MakeWeakPtrLambda(this, [this]
					{
						const FVoxelGraphBlueprintOutput& CurrentOutput = Cast<UK2Node_QueryVoxelGraphOutput>(GraphPinObj->GetOwningNode())->CachedGraphOutput;
						return FVoxelSelectorItem(CurrentOutput.Guid, CurrentOutput.Name, CurrentOutput.Type);
					}))
					.IsValidItem_Lambda(MakeWeakPtrLambda(this, [this]
					{
						const FVoxelGraphBlueprintOutput& CurrentOutput = Cast<UK2Node_QueryVoxelGraphOutput>(GraphPinObj->GetOwningNode())->CachedGraphOutput;
						return CurrentOutput.bIsValid;
					}))
					.OnItemChanged(MakeWeakPtrDelegate(this, [this](const FVoxelSelectorItem& NewItem)
					{
						if (!ensure(!GraphPinObj->IsPendingKill()) ||
							!GraphPinObj->GetOwningNode())
						{
							return;
						}

						const FVoxelTransaction Transaction(GraphPinObj, "Change Graph Output Pin Value");
						GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, NewItem.Guid.ToString());
					}))
				]
			]
		];
}