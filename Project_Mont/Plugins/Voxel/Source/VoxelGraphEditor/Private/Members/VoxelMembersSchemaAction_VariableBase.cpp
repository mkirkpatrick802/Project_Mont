// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Members/VoxelMembersSchemaAction_VariableBase.h"
#include "Members/SVoxelGraphMembers.h"
#include "Widgets/SVoxelGraphPinTypeComboBox.h"

TSharedRef<SWidget> FVoxelMembersSchemaAction_VariableBase::CreatePaletteWidget(const FCreateWidgetForActionData& CreateData) const
{
	return
		SNew(SVoxelMembersPaletteItem_VariableBase, CreateData)
		.MembersWidget(GetMembers());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelMembersPaletteItem_VariableBase::Construct(const FArguments& InArgs, const FCreateWidgetForActionData& CreateData)
{
	WeakMembersWidget = InArgs._MembersWidget;
	ActionPtr = CreateData.Action;

	const TSharedRef<FVoxelColumnSizeData> ColumnSizeData = InArgs._MembersWidget->ColumnSizeData;
	bool bIsInherited;
	bool bIsParameters;
	FVoxelPinType Type;
	int32 CategoriesCount;
	{
		const TSharedPtr<FVoxelMembersSchemaAction_VariableBase> Action = GetAction();
		if (!ensure(Action))
		{
			return;
		}

		Type = Action->GetPinType();
		bIsInherited = Action->bIsInherited;
		bIsParameters = SVoxelGraphMembers::GetSection(Action->SectionID) == EVoxelGraphMemberSection::Parameters;
		CategoriesCount = FVoxelUtilities::ParseCategory(Action->GetCategory()).Num();
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
					SNew(SHorizontalBox)
					.Clipping(EWidgetClipping::ClipToBounds)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						CreateTextSlotWidget(CreateData)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5.f, 0.f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Visibility(bIsInherited ? EVisibility::Visible : EVisibility::Collapsed)
						.Text(INVTEXT("inherited"))
						.ToolTipText(INVTEXT("This member is inherited from a parent graph"))
						.Font(FCoreStyle::GetDefaultFontStyle("Italic", 7))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			]
			+ SSplitter::Slot()
			.Value_Lambda([=]
			{
				return ColumnSizeData->GetValueWidth(CategoriesCount, GetCachedGeometry().GetAbsoluteSize().X);
			})
			.OnSlotResized_Lambda([=](const float NewValue)
			{
				ColumnSizeData->SetValueWidth(
					NewValue,
					CategoriesCount,
					GetCachedGeometry().GetAbsoluteSize().X);
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
						const TSharedPtr<FVoxelMembersSchemaAction_VariableBase> Action = GetAction();
						if (!ensure(Action))
						{
							return;
						}

						Action->SetPinType(NewValue);
					})
					.AllowedTypes(bIsParameters ? FVoxelPinTypeSet::AllUniformsAndBufferArrays() : FVoxelPinTypeSet::All())
					.DetailsWindow(false)
					.ReadOnly_Lambda([=]
					{
						return bIsInherited || !IsHovered();
					})
				]
			]
		]
	];
}