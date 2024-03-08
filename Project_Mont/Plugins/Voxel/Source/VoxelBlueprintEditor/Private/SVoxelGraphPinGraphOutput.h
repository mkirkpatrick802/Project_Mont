// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraph.h"

class SVoxelGraphParameterComboBox;
struct FVoxelGraphBlueprintOutput;

class SVoxelGraphPinGraphOutput : public SGraphPin
{
public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

	//~ Begin SGraphPin Interface
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;
	//~ End SGraphPin Interface

private:
	TWeakObjectPtr<UVoxelGraph> WeakGraph;
	FEdGraphPinReference AssetPinReference;
	TSharedPtr<SVoxelGraphParameterComboBox> ParameterComboBox;

	TSharedPtr<SBox> TextContainer;
	TSharedPtr<SBox> ParameterSelectorContainer;
};