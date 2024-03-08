// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Material/VoxelMaterialDefinitionParameterSchemaAction.h"
#include "Material/SVoxelMaterialDefinitionParameters.h"
#include "Material/VoxelMaterialDefinitionToolkit.h"
#include "Widgets/SVoxelGraphPinTypeComboBox.h"

FEdGraphSchemaActionDefiningObject FVoxelMaterialDefinitionParameterSchemaAction::GetPersistentItemDefiningObject() const
{
	return FEdGraphSchemaActionDefiningObject(MaterialDefinition.Get());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelMaterialDefinitionParameterPaletteItem::Construct(const FArguments& InArgs, FCreateWidgetForActionData* CreateData)
{
	ActionPtr = CreateData->Action;

	const TSharedPtr<SVoxelMaterialDefinitionParameters> Parameters = InArgs._Parameters;
	const TSharedRef<FVoxelColumnSizeData> ColumnSizeData = Parameters->ColumnSizeData;

	WeakToolkit = Parameters->GetToolkit();

	FVoxelPinType Type;
	int32 CategoriesCount;
	{
		const TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> Action = GetAction();
		if (!ensure(Action))
		{
			return;
		}

		Type = GetPinType();
		CategoriesCount = FVoxelUtilities::ParseCategory(Action->GetCategory().ToString()).Num();
	}

	ChildSlot
	[
		SNew(SBox)
		.Padding(FMargin(0.f, -2.f))
		[
			SNew(SSplitter)
			.Style(FVoxelEditorStyle::Get(), "Members.Splitter")
			.PhysicalSplitterHandleSize(1.f)
			.HitDetectionSplitterHandleSize(5.f)
			.HighlightedHandleIndex(ColumnSizeData, &FVoxelColumnSizeData::GetHoveredSplitterIndex)
			.OnHandleHovered(ColumnSizeData, &FVoxelColumnSizeData::SetHoveredSplitterIndex)
			+ SSplitter::Slot()
			.Value_Lambda([=]
			{
				return ColumnSizeData->GetNameWidth(CategoriesCount, GetCachedGeometry().GetAbsoluteSize().X);
			})
			[
				SNew(SBox)
				.Padding(FMargin(0.f, 2.f, 4.f, 2.f))
				[
					CreateTextSlotWidget(CreateData, false)
				]
			]
			+ SSplitter::Slot()
			.Value_Lambda([=]
			{
				return ColumnSizeData->GetValueWidth(CategoriesCount, GetCachedGeometry().GetAbsoluteSize().X);
			})
			.OnSlotResized_Lambda([=](const float NewValue)
			{
				ColumnSizeData->SetValueWidth(NewValue, CategoriesCount, GetCachedGeometry().GetAbsoluteSize().X);
			})
			[
				SNew(SBox)
				.Padding(FMargin(8.f, 2.f, 0.f, 2.f))
				.HAlign(HAlign_Left)
				[
					SNew(SVoxelPinTypeComboBox)
					.CurrentType(Type)
					.OnTypeChanged_Lambda([this](const FVoxelPinType NewValue)
					{
						const TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> Action = GetAction();
						if (!ensure(Action))
						{
							return;
						}

						SetPinType(NewValue);
					})
					.AllowedTypes(FVoxelPinTypeSet::AllMaterials())
					.DetailsWindow(false)
					.ReadOnly_Lambda([this]
					{
						return !IsHovered();
					})
				]
			]
		]
	];
}

TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> SVoxelMaterialDefinitionParameterPaletteItem::GetAction() const
{
	const TSharedPtr<FEdGraphSchemaAction> Action = ActionPtr.Pin();
	if (!Action)
	{
		return nullptr;
	}

	return StaticCastSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction>(Action);
}

void SVoxelMaterialDefinitionParameterPaletteItem::OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	const TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> Action = GetAction();
	if (!ensure(Action))
	{
		return;
	}

	const TSharedPtr<FVoxelMaterialDefinitionToolkit> Toolkit = WeakToolkit.Pin();
	if (!ensure(Toolkit))
	{
		return;
	}
	UVoxelMaterialDefinition* MaterialDefinition = Toolkit->Asset;


	FVoxelMaterialDefinitionParameter* Parameter = MaterialDefinition->GuidToMaterialParameter.Find(Action->Guid);
	if (!ensure(Parameter))
	{
		return;
	}

	{
		const FVoxelTransaction Transaction(Toolkit->Asset, "Rename parameter");
		Parameter->Name = FName(NewText.ToString());
	}

	Toolkit->SelectParameter(Action->Guid, false, true);
}

FText SVoxelMaterialDefinitionParameterPaletteItem::GetDisplayText() const
{
	const TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> Action = GetAction();
	if (!ensure(Action))
	{
		return {};
	}

	const TSharedPtr<FVoxelMaterialDefinitionToolkit> Toolkit = WeakToolkit.Pin();
	if (!ensure(Toolkit))
	{
		return {};
	}
	UVoxelMaterialDefinition* MaterialDefinition = Toolkit->Asset;

	const FVoxelMaterialDefinitionParameter* Parameter = MaterialDefinition->GuidToMaterialParameter.Find(Action->Guid);
	if (!ensure(Parameter))
	{
		return {};
	}

	return FText::FromName(Parameter->Name);
}

FVoxelPinType SVoxelMaterialDefinitionParameterPaletteItem::GetPinType() const
{
	const TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> Action = GetAction();
	if (!ensure(Action))
	{
		return {};
	}

	const TSharedPtr<FVoxelMaterialDefinitionToolkit> Toolkit = WeakToolkit.Pin();
	if (!ensure(Toolkit))
	{
		return {};
	}
	UVoxelMaterialDefinition* MaterialDefinition = Toolkit->Asset;

	const FVoxelMaterialDefinitionParameter* Parameter = MaterialDefinition->GuidToMaterialParameter.Find(Action->Guid);
	if (!ensure(Parameter))
	{
		return {};
	}

	return Parameter->Type;
}

void SVoxelMaterialDefinitionParameterPaletteItem::SetPinType(const FVoxelPinType& NewPinType) const
{
	// Materials don't support array
	ensure(!NewPinType.IsBuffer());

	const TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> Action = GetAction();
	if (!ensure(Action))
	{
		return;
	}

	const TSharedPtr<FVoxelMaterialDefinitionToolkit> Toolkit = WeakToolkit.Pin();
	if (!ensure(Toolkit))
	{
		return;
	}
	UVoxelMaterialDefinition* MaterialDefinition = Toolkit->Asset;

	FVoxelMaterialDefinitionParameter* Parameter = MaterialDefinition->GuidToMaterialParameter.Find(Action->Guid);
	if (!ensure(Parameter))
	{
		return;
	}

	{
		const FVoxelTransaction Transaction(Toolkit->Asset, "Change parameter type");
		Parameter->Type = NewPinType;
	}

	Toolkit->SelectParameter(Action->Guid, false, true);
}