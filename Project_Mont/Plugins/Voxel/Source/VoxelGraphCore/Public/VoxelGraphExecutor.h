// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelGraphNodeRef.h"

class FVoxelRuntime;
class FVoxelTerminalGraphInstance;
class FVoxelExecNodeRuntimeWrapper;

class VOXELGRAPHCORE_API FVoxelGraphExecutor : public TSharedFromThis<FVoxelGraphExecutor>
{
public:
	const FVoxelTerminalGraphRef TerminalGraphRef;
	const TSharedRef<FVoxelTerminalGraphInstance> TerminalGraphInstance;

	VOXEL_COUNT_INSTANCES();

public:
	static TSharedRef<FVoxelGraphExecutor> Create_GameThread(
		const FVoxelTerminalGraphRef& TerminalGraphRef,
		const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance);

	void Tick(FVoxelRuntime& Runtime) const;
	void AddReferencedObjects(FReferenceCollector& Collector);
	FVoxelOptionalBox GetBounds() const;

private:
	struct FExecNodeKey
	{
		TWeakObjectPtr<const UVoxelTerminalGraph> WeakTerminalGraph;
		FName NodeId;

		FORCEINLINE bool operator==(const FExecNodeKey& Other) const
		{
			return
				WeakTerminalGraph == Other.WeakTerminalGraph &&
				NodeId == Other.NodeId;
		}
		FORCEINLINE friend uint32 GetTypeHash(const FExecNodeKey& Key)
		{
			return
				GetTypeHash(Key.WeakTerminalGraph) ^
				GetTypeHash(Key.NodeId);
		}
	};
#if WITH_EDITOR
	FSharedVoidPtr OnChangedPtr;
#endif
	TMap<FExecNodeKey, TSharedPtr<FVoxelExecNodeRuntimeWrapper>> ExecNodeKeyToWrapper_GameThread;

	FVoxelGraphExecutor(
		const FVoxelTerminalGraphRef& TerminalGraphRef,
		const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance)
		: TerminalGraphRef(TerminalGraphRef)
		, TerminalGraphInstance(TerminalGraphInstance)
	{
	}

	void Update_GameThread();
};