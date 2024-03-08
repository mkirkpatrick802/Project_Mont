// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Members/VoxelMembersSchemaAction_Function.h"

struct FVoxelMembersSchemaAction_LibraryFunction : public FVoxelMembersSchemaAction_Function
{
public:
	using FVoxelMembersSchemaAction_Function::FVoxelMembersSchemaAction_Function;

	//~ Begin FVoxelMembersSchemaAction_Base Interface
	virtual TSharedRef<SWidget> CreatePaletteWidget(const FCreateWidgetForActionData& CreateData) const override;
	//~ End FVoxelMembersSchemaAction_Base Interface

	bool IsFunctionExposed() const;
	void ToggleFunctionExposure() const;
};

class SVoxelMembersPaletteItem_LibraryFunction : public SVoxelMembersPaletteItem<FVoxelMembersSchemaAction_LibraryFunction>
{
public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& InArgs, const FCreateWidgetForActionData& CreateData);

private:
	TSharedRef<SWidget> CreateVisibilityWidget() const;
};