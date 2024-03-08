// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Widgets/SVoxelGraphSearch.h"

class SVoxelGraphSearch;

class FVoxelGraphSearchManager : public FVoxelSingleton
{
public:
	const FName GlobalSearchTabId = "VoxelGraphSearch";

	FVoxelGraphSearchManager() = default;

	//~ Begin FVoxelSingleton Interface
	virtual void Initialize() override;
	//~ End FVoxelSingleton Interface

	TSharedPtr<SVoxelGraphSearch> OpenGlobalSearch() const;
};
extern FVoxelGraphSearchManager* GVoxelGraphSearchManager;