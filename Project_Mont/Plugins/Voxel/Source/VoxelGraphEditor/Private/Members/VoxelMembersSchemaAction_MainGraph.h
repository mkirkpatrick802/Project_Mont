// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Members/VoxelMembersSchemaAction_Base.h"

class UVoxelGraph;

struct FVoxelMembersSchemaAction_MainGraph : public FVoxelMembersSchemaAction_Base
{
public:
	TWeakObjectPtr<UVoxelGraph> WeakGraph;

	using FVoxelMembersSchemaAction_Base::FVoxelMembersSchemaAction_Base;

	//~ Begin FVoxelMembersSchemaAction_Base Interface
	virtual UObject* GetOuter() const override;
	virtual TSharedRef<SWidget> CreatePaletteWidget(const FCreateWidgetForActionData& CreateData) const override;
	virtual void BuildContextMenu(FMenuBuilder& MenuBuilder) override;
	virtual void ApplyNewGuids(const TArray<FGuid>& NewGuids) const override;
	virtual void OnActionDoubleClick() const override;
	virtual void OnSelected() const override;
	virtual FString GetName() const override;
	virtual bool CanBeRenamed() const override { return false; }
	//~ End FVoxelMembersSchemaAction_Base Interface
};

class SVoxelMembersPaletteItem_MainGraph : public SVoxelMembersPaletteItem<SVoxelMembersPaletteItem_MainGraph>
{
public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& InArgs, const FCreateWidgetForActionData& CreateData);
};