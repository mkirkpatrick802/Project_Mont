// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelCurveEditorData.h"
#include "CurveDataAbstraction.h"

class SVoxelCurvePanel;

class FVoxelCurveModel : public TSharedFromThis<FVoxelCurveModel>
{
public:
	FVoxelCurveModel(const TSharedPtr<IPropertyHandle>& Handle, const FSimpleDelegate& OnInvalidate);

	TWeakPtr<IPropertyHandle> WeakHandle;
	TSharedPtr<FRichCurve> Curve;

	TSet<FKeyHandle> SelectedKeys;
	TArray<FVoxelCurveKeyPoint> CachedKeyPoints;
	FVoxelCurveKeyPoint::EKeyPointType SelectedTangentType = FVoxelCurveKeyPoint::EKeyPointType::None;

	FSlateRect CachedCurveScreenBox;

	FVoxelCurveGridSpacing HorizontalSpacing;
	FVoxelCurveGridSpacing VerticalSpacing;

	TArray<FVector2D> InterpolatedCurvePoints;

	FKeyAttributes CachedKeyAttributes;

	FVector2D CurveMin = FVector2D(INT32_MAX);
	FVector2D CurveMax = FVector2D(INT32_MIN);

	FVector2D UIMin;
	FVector2D UIMax;
	FVector2D ClampMin;
	FVector2D ClampMax;

	FSimpleDelegate OnInvalidate;

	bool bExtendByTangents = false;

public:
	void UpdatePanelRanges();
	void BindCommands(const TSharedPtr<FUICommandList>& CommandList);

	void RefreshCurve();
	void SetCurve(const FRichCurve& InCurve) const;
	void SaveCurve() const;

	float GetHorizontalPosition(float Input) const;
	float GetVerticalPosition(float Output) const;

	void PrepareData(const FGeometry& AllottedGeometry, const SVoxelCurvePanel& Panel);

	void SetKeyAttributes(FKeyAttributes KeyAttributes);

	float GetPixelsPerInput() const;
	float GetPixelsPerOutput() const;
	FSlateRect GetSnapDimensions() const;
	FVector2D GetSnapIntervals() const;

	void AddKeyAtHoveredPosition(TWeakPtr<SVoxelCurvePanel> WeakPanel);
	void AddKey(float Input, float Output);
	void DeleteSelectedKeys();
	void CutSelectedKeys();
	void CopySelectedKeys();
	void PasteKeys();

	void FlattenTangents();
	void StraightenTangents();

	void SelectAllKeys();
	void DeselectAllKeys();
	void SelectNextKey();
	void SelectPreviousKey();

	void SelectHoveredKey();
	void UpdateCachedKeyAttributes();
	void SelectKeys(const TSet<FKeyHandle>& Handles);
	void SelectKey(const FKeyHandle& Handle);

	TOptional<float> GetSelectedKeysInput() const;
	void SetSelectedKeysInput(float NewValue, ETextCommit::Type Type);
	TOptional<float> GetSelectedKeysOutput() const;
	void SetSelectedKeysOutput(float NewValue, ETextCommit::Type Type);

	bool HasSelectedKeys() const { return SelectedKeys.Num() > 0; }
	EVisibility HasSelectedKeys(const bool bInvert) const { return !bInvert == HasSelectedKeys() ? EVisibility::Visible : EVisibility::Collapsed; }

	void ExtendByTangents();
	bool IsExtendByTangentsEnabled() const { return bExtendByTangents; }
};
