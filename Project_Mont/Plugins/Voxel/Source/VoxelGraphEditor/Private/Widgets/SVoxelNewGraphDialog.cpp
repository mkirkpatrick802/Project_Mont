// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelNewGraphDialog.h"
#include "VoxelContentEditorModule.h"
#include "SVoxelNewAssetInstancePickerList.h"

#include "ToolMenus.h"

void SVoxelNewGraphDialog::Construct(const FArguments& InArgs, UClass* TargetClass)
{
	SWindow::Construct(
		SWindow::FArguments()
		.Title(FText::FromString("Pick a starting point for your " + TargetClass->GetDisplayNameText().ToString()))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(450.f, 600.f))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
			.Padding(10.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(1.f)
				[
					SNew(SBox)
					.HAlign(HAlign_Fill)
					[
						SAssignNew(NewAssetPicker, SVoxelNewAssetInstancePickerList, TargetClass)
						.OnTemplateAssetActivated(this, &SVoxelNewGraphDialog::ConfirmSelection)
						.OnGetAssetCategory(InArgs._OnGetAssetCategory)
						.OnGetAssetDescription(InArgs._OnGetAssetDescription)
						.ShowAsset(InArgs._ShowAsset)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.Padding(0.f, 10.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.HAlign(HAlign_Left)
					.Padding(FCoreStyle::Get().GetMargin("StandardDialog.SlotPadding"))
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SecondaryButton"))
						.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("DialogButtonText"))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ContentPadding(FCoreStyle::Get().GetMargin("StandardDialog.ContentPadding"))
						.OnClicked(this, &SVoxelNewGraphDialog::OnOpenContentExamplesClicked)
						.ToolTipText(INVTEXT("Open Voxel Plugin content examples"))
						.Text(INVTEXT("Content Examples"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FCoreStyle::Get().GetMargin("StandardDialog.SlotPadding"))
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
						.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("DialogButtonText"))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ContentPadding(FCoreStyle::Get().GetMargin("StandardDialog.ContentPadding"))
						.IsEnabled(this, &SVoxelNewGraphDialog::IsCreateButtonEnabled)
						.OnClicked(this, &SVoxelNewGraphDialog::OnConfirmSelection)
						.ToolTipText(INVTEXT("Create graph from selected template"))
						.Text(INVTEXT("Create"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FCoreStyle::Get().GetMargin("StandardDialog.SlotPadding"))
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"))
						.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("DialogButtonText"))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ContentPadding(FCoreStyle::Get().GetMargin("StandardDialog.ContentPadding"))
						.OnClicked(this, &SVoxelNewGraphDialog::OnCancelButtonClicked)
						.Text(INVTEXT("Cancel"))
					]
				]
			]
		]);

	FSlateApplication::Get().SetKeyboardFocus(NewAssetPicker);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TOptional<FAssetData> SVoxelNewGraphDialog::GetSelectedAsset()
{
	if (SelectedAssets.Num() > 0)
	{
		return SelectedAssets[0];
	}

	return {};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelNewGraphDialog::ConfirmSelection(const FAssetData& AssetData)
{
	SelectedAssets.Add(AssetData);

	bUserConfirmedSelection = true;
	RequestDestroyWindow();
}

FReply SVoxelNewGraphDialog::OnConfirmSelection()
{
	SelectedAssets = NewAssetPicker->GetSelectedAssets();
	ensureMsgf(!SelectedAssets.IsEmpty(), TEXT("No assets selected when dialog was confirmed."));

	bUserConfirmedSelection = true;
	RequestDestroyWindow();

	return FReply::Handled();
}

bool SVoxelNewGraphDialog::IsCreateButtonEnabled() const
{
	return !NewAssetPicker->GetSelectedAssets().IsEmpty();
}

FReply SVoxelNewGraphDialog::OnCancelButtonClicked()
{
	bUserConfirmedSelection = false;
	SelectedAssets.Empty();

	RequestDestroyWindow();

	return FReply::Handled();
}

FReply SVoxelNewGraphDialog::OnOpenContentExamplesClicked()
{
	IVoxelContentEditorModule* ContentEditor = FModuleManager::Get().LoadModulePtr<IVoxelContentEditorModule>("VoxelContentEditor");
	if (ensure(ContentEditor))
	{
		RequestDestroyWindow();
		ContentEditor->ShowContent();
	}

	return FReply::Handled();
}