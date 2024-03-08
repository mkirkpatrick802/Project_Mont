// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphNode_Struct_Customization.h"
#include "VoxelGraph.h"
#include "Nodes/VoxelGraphNode_Struct.h"
#include "VoxelGraphNodeVariadicPinCustomization.h"
#include "Customizations/VoxelPinValueCustomizationHelper.h"

void FVoxelGraphNode_Struct_Customization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	const TArray<TWeakObjectPtr<UObject>> SelectedObjects = DetailLayout.GetSelectedObjects();

	if (SelectedObjects.Num() != 1)
	{
		return;
	}

	UVoxelGraphNode_Struct* Node = Cast<UVoxelGraphNode_Struct>(SelectedObjects[0]);
	if (!Node ||
		!Node->Struct)
	{
		return;
	}

	const TSharedRef<IPropertyHandle> StructHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelGraphNode_Struct, Struct), Node->GetClass());

	const TSharedRef<IVoxelNodeDefinition> NodeDefinition = Node->GetNodeDefinition();
	KeepAlive(NodeDefinition);

	Node->Struct->OnExposedPinsUpdated.Add(MakeWeakPtrDelegate(this, [RefreshDelegate = FVoxelEditorUtilities::MakeRefreshDelegate(this, DetailLayout)]
	{
		RefreshDelegate.ExecuteIfBound();
	}));

	DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelGraphNode, PreviewedPin), UVoxelGraphNode::StaticClass())->MarkHiddenByCustomization();
	DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelGraphNode, PreviewSettings), UVoxelGraphNode::StaticClass())->MarkHiddenByCustomization();

	const TMap<FName, TSharedPtr<IPropertyHandle>> ChildrenHandles = InitializeStructChildren(StructHandle);
	if (ChildrenHandles.Num() == 0)
	{
		return;
	}

	DetailLayout.HideProperty(StructHandle);

	for (const auto& It : ChildrenHandles)
	{
		if (It.Key == GET_MEMBER_NAME_STATIC(FVoxelNode, ExposedPinValues))
		{
			continue;
		}

		DetailLayout.AddPropertyToCategory(It.Value);
	}

	const TSharedPtr<IPropertyHandle> ExposedPinValuesHandle = ChildrenHandles.FindRef(GET_MEMBER_NAME_STATIC(FVoxelNode, ExposedPinValues));
	if (!ensure(ExposedPinValuesHandle))
	{
		return;
	}

	IDetailCategoryBuilder& DefaultCategoryBuilder = DetailLayout.EditCategory("Default", INVTEXT("Default"));

	TMap<FName, TSharedPtr<IPropertyHandle>> MappedHandles;

	uint32 NumValues = 0;
	ExposedPinValuesHandle->GetNumChildren(NumValues);

	for (uint32 Index = 0; Index < NumValues; Index++)
	{
		const TSharedPtr<IPropertyHandle> ValueHandle = ExposedPinValuesHandle->GetChildHandle(Index);
		if (!ensure(ValueHandle))
		{
			continue;
		}

		FName PinName;
		ValueHandle->GetChildHandleStatic(FVoxelNodeExposedPinValue, Name)->GetValue(PinName);

		if (!ensure(!PinName.IsNone()))
		{
			continue;
		}

		MappedHandles.Add(PinName, ValueHandle);
	}

	for (const FName PinName : Node->Struct->PrivatePinsOrder)
	{
		if (const TSharedPtr<FVoxelNode::FVariadicPin> VariadicPin = Node->Struct->PrivateNameToVariadicPin.FindRef(PinName))
		{
			if (!VariadicPin->PinTemplate.Metadata.bShowInDetail)
			{
				continue;
			}

			IDetailCategoryBuilder* TargetBuilder = &DefaultCategoryBuilder;
			if (!VariadicPin->PinTemplate.Metadata.Category.IsEmpty())
			{
				TargetBuilder = &DetailLayout.EditCategory(FName(VariadicPin->PinTemplate.Metadata.Category), FText::FromString(VariadicPin->PinTemplate.Metadata.Category));
			}

			TSharedRef<FVoxelGraphNodeVariadicPinCustomization> PinArrayCustomization = MakeVoxelShared<FVoxelGraphNodeVariadicPinCustomization>();
			PinArrayCustomization->PinName = PinName.ToString();
			PinArrayCustomization->Tooltip = VariadicPin->PinTemplate.Metadata.Tooltip.Get(); // TODO:
			PinArrayCustomization->WeakCustomization = SharedThis(this);
			PinArrayCustomization->WeakStructNode = Node;
			PinArrayCustomization->WeakNodeDefinition = NodeDefinition;

			for (FName ArrayElementPinName : VariadicPin->Pins)
			{
				if (TSharedPtr<IPropertyHandle> ValueHandle = MappedHandles.FindRef(ArrayElementPinName))
				{
					PinArrayCustomization->Properties.Add(ValueHandle);
				}
			}

			TargetBuilder->AddCustomBuilder(PinArrayCustomization);
			continue;
		}

		TSharedPtr<FVoxelPin> VoxelPin = Node->Struct->FindPin(PinName);
		if (!VoxelPin)
		{
			continue;
		}

		if (!VoxelPin->Metadata.bShowInDetail)
		{
			continue;
		}

		if (!VoxelPin->VariadicPinName.IsNone())
		{
			continue;
		}

		IDetailCategoryBuilder* TargetBuilder = &DefaultCategoryBuilder;
		if (!VoxelPin->Metadata.Category.IsEmpty())
		{
			TargetBuilder = &DetailLayout.EditCategory(FName(VoxelPin->Metadata.Category), FText::FromString(VoxelPin->Metadata.Category));
		}

		TSharedPtr<IPropertyHandle> ValueHandle = MappedHandles.FindRef(PinName);
		if (!ValueHandle)
		{
			continue;
		}

		ValueHandle->SetPropertyDisplayName(FText::FromString(VoxelPin->Metadata.DisplayName.IsEmpty() ? FName::NameToDisplayString(PinName.ToString(), false) : FName::NameToDisplayString(VoxelPin->Metadata.DisplayName, false)));

		const auto SetupRowLambda = [this, ValueHandle, WeakNode = MakeWeakObjectPtr(Node), PinName](FDetailWidgetRow& Row, const TSharedRef<SWidget>& ValueWidget)
		{
			Row
			.NameContent()
			[
				ValueHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			.HAlign(HAlign_Fill)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.Visibility_Lambda([WeakNode, PinName]
					{
						const UVoxelGraphNode_Struct* TargetNode = WeakNode.Get();
						if (!ensure(TargetNode))
						{
							return EVisibility::Visible;
						}

						return TargetNode->Struct->ExposedPins.Contains(PinName) ? EVisibility::Collapsed : EVisibility::Visible;
					})
					.MinDesiredWidth(125.f)
					[
						ValueWidget
					]
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.FillWidth(1.f)
				[
					CreateExposePinButton(WeakNode, PinName)
				]
			];
		};

		KeepAlive(FVoxelPinValueCustomizationHelper::CreatePinValueCustomization(
			ValueHandle->GetChildHandleStatic(FVoxelNodeExposedPinValue, Value),
			*TargetBuilder,
			FVoxelEditorUtilities::MakeRefreshDelegate(this, DetailLayout),
			{},
			SetupRowLambda,
			// Used to load/save expansion state
			FAddPropertyParams().UniqueId("FVoxelGraphNodeCustomization")));
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> FVoxelGraphNode_Struct_Customization::CreateExposePinButton(const TWeakObjectPtr<UVoxelGraphNode_Struct>& WeakNode, FName PinName) const
{
	return
		PropertyCustomizationHelpers::MakeVisibilityButton(
			FOnClicked::CreateLambda([WeakNode, PinName]() -> FReply
			{
				UVoxelGraphNode_Struct* TargetNode = WeakNode.Get();
				if (!ensure(TargetNode))
				{
					return FReply::Handled();
				}

				const FVoxelTransaction Transaction(TargetNode, "Expose Pin");

				if (TargetNode->Struct->ExposedPins.Remove(PinName) == 0)
				{
					TargetNode->Struct->ExposedPins.Add(PinName);
				}

				TargetNode->ReconstructNode(false);
				return FReply::Handled();
			}),
			{},
			MakeAttributeLambda([WeakNode, PinName]
			{
				const UVoxelGraphNode_Struct* TargetNode = WeakNode.Get();
				if (!ensure(TargetNode))
				{
					return false;
				}

				return TargetNode->Struct->ExposedPins.Contains(PinName) ? true : false;
			}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TMap<FName, TSharedPtr<IPropertyHandle>> FVoxelGraphNode_Struct_Customization::InitializeStructChildren(const TSharedRef<IPropertyHandle>& StructHandle)
{
	const TSharedPtr<FVoxelInstancedStructDetailsWrapper> Wrapper = FVoxelInstancedStructDetailsWrapper::Make(StructHandle);
	if (!Wrapper)
	{
		return {};
	}

	KeepAlive(Wrapper);

	TMap<FName, TSharedPtr<IPropertyHandle>> MappedChildHandles;
	for (const TSharedPtr<IPropertyHandle>& ChildHandle : Wrapper->AddChildStructure())
	{
		MappedChildHandles.Add(ChildHandle->GetProperty()->GetFName(), ChildHandle);
	}
	return MappedChildHandles;
}