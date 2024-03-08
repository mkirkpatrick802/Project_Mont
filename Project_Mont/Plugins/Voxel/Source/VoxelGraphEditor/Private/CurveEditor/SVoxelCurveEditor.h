// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SVoxelCurvePanel.h"
#include "Curves/CurveFloat.h"

class VOXELGRAPHEDITOR_API SVoxelCurveEditor : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		FArguments()
			: _CurveTint(FLinearColor(1.f, 0.05f, 0.05f, 1.f))
			, _KeysTint(FLinearColor(1.f, 0.05f, 0.05f, 1.f))
			, _TangentsTint(FLinearColor(0.2f, 0.2f, 0.2f))
			, _MinimumViewPanelHeight(300.0f)
		{}

		SLATE_ARGUMENT(TSharedPtr<IPropertyHandle>, Handle)
		SLATE_ARGUMENT(FLinearColor, CurveTint)
		SLATE_ARGUMENT(FLinearColor, KeysTint)
		SLATE_ARGUMENT(FLinearColor, TangentsTint)
		SLATE_ARGUMENT(float, MinimumViewPanelHeight)
	};

	void Construct(const FArguments& InArgs);

	//~ Begin SCompoundWidget Interface
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	//~ End SCompoundWidget Interface

	void RefreshCurve() const;

private:
	void InvalidateRetainer() const;

private:
	TSharedRef<SWidget> MakeTopToolbar();
	TSharedRef<SWidget> MakeBottomToolbar();
	void CopySelectedCurve(const FAssetData& AssetData) const;
	void SaveCurveToAsset() const;

	TSharedRef<SWidget> MakeSnapMenu(bool bInput);
	TSharedRef<SWidget> MakeAxisSnappingMenu() const;
	TSharedRef<SWidget> MakeTemplatesMenu();

	FReply OnChangeCurveTemplate(TWeakObjectPtr<UCurveFloat> WeakFloatCurveAsset) const;

private:
	TSharedPtr<SVoxelCurvePanel> CurvePanel;
	TSharedPtr<SRetainerWidget> RetainerWidget;
	TSharedPtr<FVoxelCurveModel> CurveModel;

	TSharedPtr<FUICommandList> CommandList;
};