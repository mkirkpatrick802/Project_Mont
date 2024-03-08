// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

class SVoxelAddContentTile : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		FArguments()
			: _ThumbnailPadding(FMargin(5.0f, 0))
			, _IsSelected(false)
		{

		}

		SLATE_ATTRIBUTE(const FSlateBrush*, Image)
		SLATE_ATTRIBUTE(FText, DisplayName)
		SLATE_ARGUMENT(TOptional<FVector2D>, ImageSize)
		SLATE_ARGUMENT(FMargin, ThumbnailPadding)
		SLATE_ATTRIBUTE(bool, IsSelected)
		SLATE_ARGUMENT(TSet<FName>, ContentTags)
		SLATE_ATTRIBUTE(TSet<FName>, VisibleTags)
	};

	void Construct(const FArguments& InArgs);

private:
	TAttribute<const FSlateBrush*> Image;
	TAttribute<FText> Text;
	TAttribute<bool> IsSelected;
	TAttribute<TSet<FName>> VisibleTags;
};