// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelPinType.h"

class SVoxelToolTip : public SToolTip
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ATTRIBUTE(FText, Text)
		SLATE_ATTRIBUTE(FVoxelPinType, PinType)
	};

	void Construct(const FArguments& InArgs);

	//~ Begin SToolTip Interface
	virtual void OnOpening() override;
	virtual bool IsInteractive() const override;
	//~ End SToolTip Interface

private:
	TSharedRef<SWidget> CreateToolTipWidget() const;

private:
	TAttribute<FVoxelPinType> PinType;
};