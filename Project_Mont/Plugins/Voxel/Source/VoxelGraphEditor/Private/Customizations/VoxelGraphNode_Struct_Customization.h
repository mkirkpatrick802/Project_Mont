// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

class IVoxelNodeDefinition;
class UVoxelGraphNode_Struct;

class FVoxelGraphNode_Struct_Customization : public FVoxelDetailCustomization
{
public:
	FVoxelGraphNode_Struct_Customization() = default;

	//~ Begin IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	//~ End IDetailCustomization Interface

	TSharedRef<SWidget> CreateExposePinButton(const TWeakObjectPtr<UVoxelGraphNode_Struct>& WeakNode, FName PinName) const;

private:
	TMap<FName, TSharedPtr<IPropertyHandle>> InitializeStructChildren(const TSharedRef<IPropertyHandle>& StructHandle);
};