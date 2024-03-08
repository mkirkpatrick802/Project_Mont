// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

struct FVoxelGraphToolkit;

class FVoxelGraphCommands : public TVoxelCommands<FVoxelGraphCommands>
{
public:
	TSharedPtr<FUICommandInfo> Compile;
	TSharedPtr<FUICommandInfo> CompileAll;
	TSharedPtr<FUICommandInfo> EnableStats;
	TSharedPtr<FUICommandInfo> EnableRangeStats;
	TSharedPtr<FUICommandInfo> FindInGraph;
	TSharedPtr<FUICommandInfo> FindInGraphs;
	TSharedPtr<FUICommandInfo> ReconstructAllNodes;
	TSharedPtr<FUICommandInfo> ToggleDebug;
	TSharedPtr<FUICommandInfo> TogglePreview;

	virtual void RegisterCommands() override;
};

class FVoxelGraphCommandManager
{
public:
	FVoxelGraphToolkit& Toolkit;

	explicit FVoxelGraphCommandManager(FVoxelGraphToolkit& Toolkit)
		: Toolkit(Toolkit)
	{
	}

	void MapActions() const;

public:
	void PasteNodes(const FString& TextToImport) const;
	void PasteNodes(
		const FVector2D& Location,
		const FString& TextToImport) const;

	void DeleteNodes(const TArray<UEdGraphNode*>& Nodes) const;
};