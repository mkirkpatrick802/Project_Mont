// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

class UVoxelGraphNode_Input;

class FVoxelGraphSelectionCustomization_Input : public FVoxelDetailCustomization
{
public:
	const FGuid Guid;
	const TWeakObjectPtr<UVoxelGraphNode_Input> WeakInputNode;

	enum class EType
	{
		FunctionInput,
		GraphInput,
		Node
	};
	const EType Type;

	FVoxelGraphSelectionCustomization_Input(
		const FGuid& Guid,
		UVoxelGraphNode_Input* InputNodeForExposeDefaultPin,
		const EType Type)
		: Guid(Guid)
		, WeakInputNode(InputNodeForExposeDefaultPin)
		, Type(Type)
	{
	}

	//~ Begin IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	//~ End IDetailCustomization Interface
};