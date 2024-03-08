// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Curves/CurveFloat.h"

class FCurveEditor;

class SVoxelCurveTemplateBar : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& InArgs, const TSharedRef<FCurveEditor>& InCurveEditor);

private:
	FReply CurveTemplateClicked(TWeakObjectPtr<UCurveFloat> FloatCurveAssetWeak) const;

private:
	TSharedPtr<FCurveEditor> CurveEditor;
};