// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

class IVoxelNodeDefinition;
class UVoxelGraphNode_Struct;
struct FVoxelGraphToolkit;

class FVoxelGraphNode_Struct_Customization;

class FVoxelGraphNodeVariadicPinCustomization
	: public IDetailCustomNodeBuilder
	, public TSharedFromThis<FVoxelGraphNodeVariadicPinCustomization>
{
public:
	FString PinName;
	FString Tooltip;
	TArray<TSharedPtr<IPropertyHandle>> Properties;
	TWeakPtr<FVoxelGraphNode_Struct_Customization> WeakCustomization;
	TWeakObjectPtr<UVoxelGraphNode_Struct> WeakStructNode;
	TWeakPtr<IVoxelNodeDefinition> WeakNodeDefinition;

	//~ Begin IDetailCustomNodeBuilder Interface
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override;
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual FName GetName() const override { return *PinName; }
	//~ End IDetailCustomNodeBuilder Interface

private:
	void AddNewPin();
	bool CanAddNewPin() const;
	void ClearAllPins();
	bool CanRemovePin() const;

	TSharedRef<SWidget> CreateElementEditButton(FName EntryPinName);

	void InsertPinBefore(FName EntryPinName);
	void DeletePin(FName EntryPinName);
	bool CanDeletePin(FName EntryPinName);
	void DuplicatePin(FName EntryPinName);
};