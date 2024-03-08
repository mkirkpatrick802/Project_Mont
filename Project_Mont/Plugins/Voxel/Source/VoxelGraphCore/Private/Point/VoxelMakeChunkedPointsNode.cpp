// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Point/VoxelMakeChunkedPointsNode.h"
#include "Point/VoxelPointSubsystem.h"
#include "VoxelTerminalGraphInstance.h"

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_MakeChunkedPoints, Out)
{
	const TValue<int32> InChunkSize = Get(ChunkSizePin, Query);

	return VOXEL_ON_COMPLETE(InChunkSize)
	{
		// Don't use the Query runtime info, use the terminal graph one
		// (pooling being done per brush makes more sense)
		const TSharedRef<FVoxelTerminalGraphInstance> TerminalGraphInstance = Query.GetSharedTerminalGraphInstance();

		const TSharedPtr<FVoxelPointSubsystem> Subsystem = TerminalGraphInstance->RuntimeInfo->FindSubsystem<FVoxelPointSubsystem>();
		if (!ensure(Subsystem))
		{
			return {};
		}

		int32 ChunkSize = InChunkSize;
		if (ChunkSize <= 0)
		{
			VOXEL_MESSAGE(Error, "{0}: Invalid chunk size {1}", this, ChunkSize);
			ChunkSize = 16;
		}

		VOXEL_SCOPE_LOCK(Subsystem->CriticalSection);

		FVoxelPointChunkProviderRef ChunkProviderRef;
		ChunkProviderRef.RuntimeProvider = TerminalGraphInstance->RuntimeInfo->GetRuntimeProvider();
		ChunkProviderRef.TerminalGraphInstanceCallstack = Query.GetTerminalGraphInstance().Callstack->Array();
		ChunkProviderRef.NodeRef = GetNodeRef();

		if (ChunkProviderRef.TerminalGraphInstanceCallstack[0].IsExplicitlyNull())
		{
			ChunkProviderRef.TerminalGraphInstanceCallstack.RemoveAt(0);
		}

		TSharedPtr<const FVoxelChunkedPointSet>& ChunkedPointSet = Subsystem->ChunkProviderToChunkedPointSet_RequiresLock.FindOrAdd(ChunkProviderRef);
		if (!ChunkedPointSet ||
			ChunkedPointSet->GetChunkSize() != ChunkSize)
		{
			ChunkedPointSet = MakeVoxelShared<FVoxelChunkedPointSet>(
				ChunkSize,
				ChunkProviderRef,
				TerminalGraphInstance,
				GetCompute(InPin, TerminalGraphInstance));
		}
		return ChunkedPointSet.ToSharedRef();
	};
}