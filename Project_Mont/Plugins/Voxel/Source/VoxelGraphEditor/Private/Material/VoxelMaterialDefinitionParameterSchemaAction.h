// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelPinType.h"
#include "SGraphPalette.h"

struct FVoxelMaterialDefinitionToolkit;
class UVoxelMaterialDefinition;
class SVoxelMaterialDefinitionParameters;

struct FVoxelMaterialDefinitionParameterSchemaAction : public FEdGraphSchemaAction
{
public:
	FGuid Guid;
	TWeakObjectPtr<UVoxelMaterialDefinition> MaterialDefinition;

	using FEdGraphSchemaAction::FEdGraphSchemaAction;

	//~ Begin FEdGraphSchemaAction Interface
	virtual FEdGraphSchemaActionDefiningObject GetPersistentItemDefiningObject() const override;
	//~ End FEdGraphSchemaAction Interface
};

class SVoxelMaterialDefinitionParameterPaletteItem : public SGraphPaletteItem
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(TSharedPtr<SVoxelMaterialDefinitionParameters>, Parameters)
	};

	void Construct(const FArguments& InArgs, FCreateWidgetForActionData* CreateData);
	TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> GetAction() const;

	//~ Begin SGraphPaletteItem Interface
	virtual void OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit) override;
	virtual FText GetDisplayText() const override;
	//~ End SGraphPaletteItem Interface

private:
	TWeakPtr<FVoxelMaterialDefinitionToolkit> WeakToolkit;

	FVoxelPinType GetPinType() const;
	void SetPinType(const FVoxelPinType& NewPinType) const;
};