// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphNodeVariadicPinCustomization.h"
#include "Nodes/VoxelGraphNode_Struct.h"
#include "VoxelGraphNode_Struct_Customization.h"
#include "Customizations/VoxelPinValueCustomizationHelper.h"

void FVoxelGraphNodeVariadicPinCustomization::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{
	const TSharedRef<SWidget> AddButton = PropertyCustomizationHelpers::MakeAddButton(
		FSimpleDelegate::CreateSP(this, &FVoxelGraphNodeVariadicPinCustomization::AddNewPin),
		INVTEXT("Add Element"),
		MakeAttributeSP(this, &FVoxelGraphNodeVariadicPinCustomization::CanAddNewPin));

	const TSharedRef<SWidget> ClearButton = PropertyCustomizationHelpers::MakeEmptyButton(
		FSimpleDelegate::CreateSP(this, &FVoxelGraphNodeVariadicPinCustomization::ClearAllPins),
		INVTEXT("Removes All Elements"),
		MakeAttributeSP(this, &FVoxelGraphNodeVariadicPinCustomization::CanRemovePin));

	NodeRow
	.NameContent()
	[
		SNew(SVoxelDetailText)
		.Text(FText::FromString(FName::NameToDisplayString(PinName, false)))
	]
	.ValueContent()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			SNew(SVoxelDetailText)
			.Text(FText::FromString(LexToString(Properties.Num()) + " Array elements"))
		]
		+ SHorizontalBox::Slot()
		.Padding(2.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			AddButton
		]
		+ SHorizontalBox::Slot()
		.Padding(2.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			ClearButton
		]
	];
}

void FVoxelGraphNodeVariadicPinCustomization::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	const TSharedPtr<FVoxelGraphNode_Struct_Customization> Customization = WeakCustomization.Pin();
	if (!ensure(Customization))
	{
		return;
	}

	int32 Index = 0;
	for (const TSharedPtr<IPropertyHandle>& Handle : Properties)
	{
		FName ElementPinName;
		Handle->GetChildHandleStatic(FVoxelNodeExposedPinValue, Name)->GetValue(ElementPinName);
		Handle->SetPropertyDisplayName(FText::FromString(LexToString(Index++)));

		const auto SetupRowLambda = [this, Handle, Customization, WeakNode = WeakStructNode, ElementPinName](FDetailWidgetRow& Row, const TSharedRef<SWidget>& ValueWidget)
		{
			Row
			.NameContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(0, 0, 3, 0)
				.AutoWidth()
				[
					SNew(SVoxelDetailText)
					.Text(INVTEXT("Index"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(0, 0, 3, 0)
				.AutoWidth()
				[
					SNew(SVoxelDetailText)
					.Text(INVTEXT("["))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(0, 0, 3, 0)
				.AutoWidth()
				[
					Handle->CreatePropertyNameWidget()
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(SVoxelDetailText)
					.Text(INVTEXT("]"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
			.ValueContent()
			.HAlign(HAlign_Fill)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.Visibility_Lambda([WeakNode, ElementPinName]
						{
							const UVoxelGraphNode_Struct* TargetNode = WeakNode.Get();
							if (!ensure(TargetNode))
							{
								return EVisibility::Visible;
							}

							return TargetNode->Struct->ExposedPins.Contains(ElementPinName) ? EVisibility::Collapsed : EVisibility::Visible;
						})
						.MinDesiredWidth(125.f)
						[
							ValueWidget
						]
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.Visibility_Lambda([WeakNode, ElementPinName]
						{
							const UVoxelGraphNode_Struct* TargetNode = WeakNode.Get();
							if (!ensure(TargetNode))
							{
								return EVisibility::Collapsed;
							}

							return TargetNode->Struct->ExposedPins.Contains(ElementPinName) ? EVisibility::Visible : EVisibility::Collapsed;
						})
						.MinDesiredWidth(125.f)
						[
							SNew(SVoxelDetailText)
							.Text(INVTEXT("Pin is exposed"))
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(4.0f, 1.0f, 0.0f, 1.0f)
				[
					CreateElementEditButton(ElementPinName)
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.FillWidth(1.f)
				[
					Customization->CreateExposePinButton(WeakNode, ElementPinName)
				]
			];
		};

		Customization->KeepAlive(FVoxelPinValueCustomizationHelper::CreatePinValueCustomization(
			Handle->GetChildHandleStatic(FVoxelNodeExposedPinValue, Value),
			ChildrenBuilder,
			FVoxelEditorUtilities::MakeRefreshDelegate(WeakCustomization.Pin().Get(), ChildrenBuilder),
			{},
			SetupRowLambda,
			{}));
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphNodeVariadicPinCustomization::AddNewPin()
{
	UVoxelGraphNode_Struct* Node = WeakStructNode.Get();
	if (!Node)
	{
		return;
	}

	if (!Node->GetNodeDefinition()->Variadic_CanAddPinTo(FName(PinName)))
	{
		return;
	}

	const FVoxelTransaction Transaction(Node, "Add Pin to Array");
	Node->GetNodeDefinition()->Variadic_AddPinTo(FName(PinName));
	Node->ReconstructNode(false);
}

bool FVoxelGraphNodeVariadicPinCustomization::CanAddNewPin() const
{
	UVoxelGraphNode_Struct* Node = WeakStructNode.Get();
	if (!Node)
	{
		return false;
	}

	return Node->GetNodeDefinition()->Variadic_CanAddPinTo(FName(PinName));
}

void FVoxelGraphNodeVariadicPinCustomization::ClearAllPins()
{
	UVoxelGraphNode_Struct* Node = WeakStructNode.Get();
	if (!Node)
	{
		return;
	}

	const FVoxelTransaction Transaction(Node, "Clear Pins Array");
	for (int32 Index = 0; Index < Properties.Num(); Index++)
	{
		if (!Node->GetNodeDefinition()->Variadic_CanRemovePinFrom(FName(PinName)))
		{
			break;
		}

		Node->GetNodeDefinition()->Variadic_RemovePinFrom(FName(PinName));
	}
	Node->ReconstructNode(false);
}

bool FVoxelGraphNodeVariadicPinCustomization::CanRemovePin() const
{
	UVoxelGraphNode_Struct* Node = WeakStructNode.Get();
	if (!Node)
	{
		return false;
	}

	return Node->GetNodeDefinition()->Variadic_CanRemovePinFrom(FName(PinName));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> FVoxelGraphNodeVariadicPinCustomization::CreateElementEditButton(const FName EntryPinName)
{
	FMenuBuilder MenuContentBuilder( true, nullptr, nullptr, true );
	{
		FUIAction InsertAction(
			FExecuteAction::CreateSP(this, &FVoxelGraphNodeVariadicPinCustomization::InsertPinBefore, EntryPinName));
		MenuContentBuilder.AddMenuEntry(INVTEXT("Insert"), {}, FSlateIcon(), InsertAction);

		FUIAction DeleteAction(
			FExecuteAction::CreateSP(this, &FVoxelGraphNodeVariadicPinCustomization::DeletePin, EntryPinName),
			FCanExecuteAction::CreateSP(this, &FVoxelGraphNodeVariadicPinCustomization::CanDeletePin, EntryPinName));
		MenuContentBuilder.AddMenuEntry(INVTEXT("Delete"), {}, FSlateIcon(), DeleteAction);

		FUIAction DuplicateAction(FExecuteAction::CreateSP(this, &FVoxelGraphNodeVariadicPinCustomization::DuplicatePin, EntryPinName));
		MenuContentBuilder.AddMenuEntry( INVTEXT("Duplicate"), {}, FSlateIcon(), DuplicateAction );
	}

	return
		SNew(SComboButton)
		.ComboButtonStyle( FAppStyle::Get(), "SimpleComboButton" )
		.ContentPadding(2)
		.ForegroundColor( FSlateColor::UseForeground() )
		.HasDownArrow(true)
		.MenuContent()
		[
			MenuContentBuilder.MakeWidget()
		];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphNodeVariadicPinCustomization::InsertPinBefore(const FName EntryPinName)
{
	const TSharedPtr<IVoxelNodeDefinition> NodeDefinition = WeakNodeDefinition.Pin();
	if (!NodeDefinition)
	{
		return;
	}

	NodeDefinition->InsertPinBefore(EntryPinName);
}

void FVoxelGraphNodeVariadicPinCustomization::DeletePin(const FName EntryPinName)
{
	const TSharedPtr<IVoxelNodeDefinition> NodeDefinition = WeakNodeDefinition.Pin();
	if (!NodeDefinition)
	{
		return;
	}

	if (!NodeDefinition->CanRemoveSelectedPin(EntryPinName))
	{
		return;
	}

	NodeDefinition->RemoveSelectedPin(EntryPinName);
}

bool FVoxelGraphNodeVariadicPinCustomization::CanDeletePin(const FName EntryPinName)
{
	const TSharedPtr<IVoxelNodeDefinition> NodeDefinition = WeakNodeDefinition.Pin();
	if (!NodeDefinition)
	{
		return false;
	}

	return NodeDefinition->CanRemoveSelectedPin(EntryPinName);
}

void FVoxelGraphNodeVariadicPinCustomization::DuplicatePin(const FName EntryPinName)
{
	const TSharedPtr<IVoxelNodeDefinition> NodeDefinition = WeakNodeDefinition.Pin();
	if (!NodeDefinition)
	{
		return;
	}

	NodeDefinition->DuplicatePin(EntryPinName);
}