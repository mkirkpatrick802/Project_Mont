// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class UVoxelTerminalGraph;
struct FVoxelGraphCompileScope;

extern VOXELGRAPHCORE_API FVoxelGraphCompileScope* GVoxelGraphCompileScope;

struct VOXELGRAPHCORE_API FVoxelGraphCompileScope
{
public:
	const UVoxelTerminalGraph& TerminalGraph;

	explicit FVoxelGraphCompileScope(
		const UVoxelTerminalGraph& TerminalGraph,
		bool bEnableLogging = true);
	~FVoxelGraphCompileScope();

	bool HasError() const
	{
		return bHasError;
	}
	const TVoxelArray<TSharedRef<FVoxelMessage>>& GetMessages() const
	{
		return Messages;
	}

private:
	FVoxelGraphCompileScope* PreviousScope = nullptr;
	bool bHasError = false;
	TVoxelArray<TSharedRef<FVoxelMessage>> Messages;
	TVoxelUniquePtr<FVoxelScopedMessageConsumer> ScopedMessageConsumer;
};