// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

class FDetailItemNode;
class FVoxelParameterView;
class FVoxelParameterDetails;

class FVoxelParameterChildBuilder
	: public IDetailCustomNodeBuilder
	, public TSharedFromThis<FVoxelParameterChildBuilder>
{
public:
	FVoxelParameterDetails& ParameterDetails;
	const TSharedRef<SWidget> ValueWidget;

	bool bIsExpanded = false;
	TWeakPtr<const FDetailItemNode> WeakNode;
	FSimpleDelegate OnRegenerateChildren;
	TVoxelArray<TVoxelArray<FVoxelParameterView*>> ParameterViewsCommonChildren;

	FVoxelParameterChildBuilder(
		FVoxelParameterDetails& ParameterDetails,
		const TSharedRef<SWidget>& ValueWidget)
		: ParameterDetails(ParameterDetails)
		, ValueWidget(ValueWidget)
	{
	}

	void UpdateExpandedState();

	//~ Begin IDetailCustomNodeBuilder Interface
	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren) override;
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& Row) override;
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual FName GetName() const override;
	//~ End IDetailCustomNodeBuilder Interface
};