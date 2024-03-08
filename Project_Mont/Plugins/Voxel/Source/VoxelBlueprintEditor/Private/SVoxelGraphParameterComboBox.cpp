// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelGraphParameterComboBox.h"
#include "VoxelGraphVisuals.h"
#include "SVoxelGraphParameterSelector.h"

void SVoxelGraphParameterComboBox::Construct(const FArguments& InArgs)
{
	OnItemChanged = InArgs._OnItemChanged;
	ensure(OnItemChanged.IsBound());

	Items = InArgs._Items;
	CurrentItem = InArgs._CurrentItem;
	bIsValidItem = InArgs._IsValidItem;

	TSharedPtr<SHorizontalBox> SelectorBox;
	ChildSlot
	[
		SNew(SBox)
		.Padding(-6.f, 0.f,0.f,0.f)
		.Clipping(EWidgetClipping::ClipToBoundsAlways)
		.HAlign(HAlign_Left)
		[
			SAssignNew(TypeComboButton, SComboButton)
			.ComboButtonStyle(FAppStyle::Get(), "ComboButton")
			.OnGetMenuContent(this, &SVoxelGraphParameterComboBox::GetMenuContent)
			.ContentPadding(0)
			.ForegroundColor_Lambda([this]
			{
				return bIsValidItem.Get() ? FSlateColor::UseForeground() : FStyleColors::Error;
			})
			.ButtonContent()
			[
				SNew(SHorizontalBox)
				.Clipping(EWidgetClipping::ClipToBoundsAlways)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				.Padding(0.f, 0.f, 2.f, 0.f)
				.AutoWidth()
				[
					SNew(SImage)
					.Image_Lambda(MakeWeakPtrLambda(this, [this]
					{
						return FVoxelGraphVisuals::GetPinIcon(CurrentItem.Get().Type).GetIcon();
					}))
					.ColorAndOpacity_Lambda(MakeWeakPtrLambda(this, [this]
					{
						return FVoxelGraphVisuals::GetPinColor(CurrentItem.Get().Type);
					}))
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				.Padding(2.f, 0.f, 0.f, 0.f)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Font(FVoxelEditorUtilities::Font())
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Text_Lambda(MakeWeakPtrLambda(this, [this]
					{
						return FText::FromName(CurrentItem.Get().Name);
					}))
				]
			]
		]
	];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SVoxelGraphParameterComboBox::GetMenuContent()
{
	if (MenuContent)
	{
		ParameterSelector->ClearSelection();

		return MenuContent.ToSharedRef();
	}

	SAssignNew(MenuContent, SMenuOwner)
	[
		SAssignNew(ParameterSelector, SVoxelGraphParameterSelector)
		.Items(Items)
		.OnItemChanged(OnItemChanged)
		.OnCloseMenu(MakeWeakPtrDelegate(this, [this]
		{
			MenuContent->CloseSummonedMenus();
			TypeComboButton->SetIsOpen(false);
		}))
	];

	TypeComboButton->SetMenuContentWidgetToFocus(ParameterSelector->GetWidgetToFocus());

	return MenuContent.ToSharedRef();
}