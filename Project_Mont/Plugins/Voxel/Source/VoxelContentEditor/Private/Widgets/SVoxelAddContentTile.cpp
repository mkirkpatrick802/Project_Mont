// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelAddContentTile.h"
#include "VoxelExampleContentManager.h"
#include "Internationalization/BreakIterator.h"

void SVoxelAddContentTile::Construct(const FArguments& InArgs)
{
	Image = InArgs._Image;
	Text = InArgs._DisplayName;
	IsSelected = InArgs._IsSelected;
	VisibleTags = InArgs._VisibleTags;

	const TSharedRef<SWrapBox> TagsContainer = SNew(SWrapBox).UseAllottedSize(true);

	TArray<FName> ContentTags = InArgs._ContentTags.Array();
	ContentTags.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
	for (const FName ContentTag : ContentTags)
	{
		const FLinearColor TagColor = FVoxelExampleContentManager::GetContentTagColor(ContentTag);

		TagsContainer->AddSlot()
		.Padding(1.f)
		[
			SNew(SImage)
			.DesiredSizeOverride(FVector2D(10.f, 6.f))
			.ColorAndOpacity_Lambda([=]
			{
				FLinearColor Color = TagColor;
				if (!VisibleTags.Get().Contains(ContentTag))
				{
					Color.A = 0.1f;
				}

				return Color;
			})
			.Image(FAppStyle::GetBrush("WhiteBrush"))
		];
	}

	ChildSlot
	[
		SNew(SBorder)
		.Padding(FMargin(0.f, 0.f, 5.f, 5.f))
		.BorderImage(FAppStyle::Get().GetBrush("ProjectBrowser.ProjectTile.DropShadow"))
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFitX)
					[
						SNew(SImage)
						.Image(Image)
						.DesiredSizeOverride(InArgs._ImageSize)
					]
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.f)
				[
					SNew(SBorder)
					.Padding(InArgs._ThumbnailPadding)
					.VAlign(VAlign_Fill)
					.Padding(FMargin(3.f, 3.f))
					.BorderImage(FAppStyle::Get().GetBrush("ProjectBrowser.ProjectTile.NameAreaBackground"))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.FillHeight(1.f)
						.VAlign(VAlign_Top)
						[
							SNew(STextBlock)
							.Font(FAppStyle::Get().GetFontStyle("ProjectBrowser.ProjectTile.Font"))
							.AutoWrapText(true)
							.LineBreakPolicy(FBreakIterator::CreateWordBreakIterator())
							.Text(InArgs._DisplayName)
							.ColorAndOpacity(FAppStyle::Get().GetSlateColor("Colors.Foreground"))
						]
						+ SVerticalBox::Slot()
						.VAlign(VAlign_Bottom)
						.HAlign(HAlign_Right)
						.AutoHeight()
						.Padding(FMargin(3.f, 3.f))
						[
							TagsContainer
						]
					]
				]
			]
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Visibility(EVisibility::HitTestInvisible)
				.Image_Lambda([this]
				{
					const bool bSelected = IsSelected.Get();
					const bool bHovered = IsHovered();

					if (bSelected && bHovered)
					{
						static const FName SelectedHover("ProjectBrowser.ProjectTile.SelectedHoverBorder");
						return FAppStyle::Get().GetBrush(SelectedHover);
					}
					else if (bSelected)
					{
						static const FName Selected("ProjectBrowser.ProjectTile.SelectedBorder");
						return FAppStyle::Get().GetBrush(Selected);
					}
					else if (bHovered)
					{
						static const FName Hovered("ProjectBrowser.ProjectTile.HoverBorder");
						return FAppStyle::Get().GetBrush(Hovered);
					}

					return FStyleDefaults::GetNoBrush();
				})
			]
		]
	];
}