// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SGraphPalette.h"

struct FVoxelGraphMembersGraphSchemaAction;

class SVoxelGraphMembersGraphPaletteItem : public SGraphPaletteItem
{
public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& InArgs, FCreateWidgetForActionData* const InCreateData);

protected:
	//~ Begin SGraphPaletteItem Interface
	virtual TSharedRef<SWidget> CreateTextSlotWidget(FCreateWidgetForActionData* const InCreateData, TAttribute<bool> bIsReadOnly) override;
	virtual FText GetDisplayText() const override;
	//~ End SGraphPaletteItem Interface

private:
	TSharedPtr<FVoxelGraphMembersGraphSchemaAction> GetAction() const;
};