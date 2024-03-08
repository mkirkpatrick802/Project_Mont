// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SGraphNode.h"
#include "Nodes/VoxelGraphNode.h"

class SVoxelGraphNodeBase : public SGraphNode
{
public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& InArgs, UVoxelGraphNode* InNode);

	UVoxelGraphNode& GetVoxelBaseNode() const
	{
		return *CastChecked<UVoxelGraphNode>(GraphNode);
	}

protected:
	//~ Begin SGraphNode Interface
	virtual TSharedPtr<SGraphPin> CreatePinWidget(UEdGraphPin* Pin) const override;
	//~ End SGraphNode Interface
};