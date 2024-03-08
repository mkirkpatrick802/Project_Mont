// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Material/VoxelMaterialDefinition.h"
#include "Material/VoxelMaterialDefinitionInterfaceToolkit.h"
#include "VoxelMaterialDefinitionToolkit.generated.h"

class SVoxelMaterialDefinitionParameters;

USTRUCT()
struct FVoxelMaterialDefinitionToolkit : public FVoxelMaterialDefinitionInterfaceToolkit
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	UPROPERTY()
	TObjectPtr<UVoxelMaterialDefinition> Asset;

public:
	//~ Begin FVoxelSimpleAssetToolkit Interface
	virtual void Initialize() override;
	virtual TSharedPtr<FTabManager::FLayout> GetLayout() const override;
	virtual void RegisterTabs(FRegisterTab RegisterTab) override;
	//~ End FVoxelSimpleAssetToolkit Interface

	void SelectParameter(const FGuid& Guid, bool bRequestRename, bool bRefresh);

private:
	static constexpr const TCHAR* ParametersTabId = TEXT("FVoxelMaterialLayerTemplateToolkit_Parameters");

	FGuid SelectedGuid;
	TSharedPtr<SVoxelMaterialDefinitionParameters> MaterialLayerParameters;
};