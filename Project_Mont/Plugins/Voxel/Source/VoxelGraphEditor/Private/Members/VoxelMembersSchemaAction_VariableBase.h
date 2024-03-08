// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelPinType.h"
#include "VoxelMembersSchemaAction_Base.h"

struct FVoxelMembersSchemaAction_VariableBase : public FVoxelMembersSchemaAction_Base
{
public:
	bool bIsInherited = false;

	using FVoxelMembersSchemaAction_Base::FVoxelMembersSchemaAction_Base;

	//~ Begin FVoxelMembersSchemaAction_Base Interface
	virtual TSharedRef<SWidget> CreatePaletteWidget(const FCreateWidgetForActionData& CreateData) const override;
	virtual bool CanBeRenamed() const override { return !bIsInherited; }
	virtual bool CanBeDeleted() const override { return !bIsInherited; }
	//~ End FVoxelMembersSchemaAction_Base Interface

	virtual FVoxelPinType GetPinType() const = 0;
	virtual void SetPinType(const FVoxelPinType& NewPinType) const = 0;
};

class SVoxelMembersPaletteItem_VariableBase : public SVoxelMembersPaletteItem<FVoxelMembersSchemaAction_VariableBase>
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(TSharedPtr<SVoxelGraphMembers>, MembersWidget)
	};

	void Construct(const FArguments& InArgs, const FCreateWidgetForActionData& CreateData);

private:
	TWeakPtr<SVoxelGraphMembers> WeakMembersWidget;
};