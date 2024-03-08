// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

struct FVoxelGraphToolkit;

class FVoxelGraphContextMenuBuilder
{
public:
	static void Build(
		UToolMenu& Menu,
		UGraphNodeContextMenuContext& Context);

private:
	UToolMenu& Menu;
	UGraphNodeContextMenuContext& Context;
	FVoxelGraphToolkit& Toolkit;

	FVoxelGraphContextMenuBuilder(
		UToolMenu& Menu,
		UGraphNodeContextMenuContext& Context,
		FVoxelGraphToolkit& Toolkit)
		: Menu(Menu)
		, Context(Context)
		, Toolkit(Toolkit)
	{
	}

	void Build() const;
};