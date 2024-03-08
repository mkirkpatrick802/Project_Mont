// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelTaskGroupScope.h"
#include "VoxelNodeStats.h"
#include "VoxelTaskGroup.h"
#include "VoxelTerminalGraphInstance.h"
#include "EdGraph/EdGraphNode.h"

#if WITH_EDITOR
class FVoxelExecNodeStatManager
	: public FVoxelSingleton
	, public IVoxelNodeStatProvider
{
public:
	struct FQueuedStat
	{
		TSharedPtr<const FVoxelCallstack> Callstack;
		double Time = 0.;
	};
	TQueue<FQueuedStat, EQueueMode::Mpsc> Queue;

	TVoxelMap<TWeakObjectPtr<const UEdGraphNode>, double> NodeToTime;

	//~ Begin FVoxelSingleton Interface
	virtual void Initialize() override
	{
		GVoxelNodeStatProviders.Add(this);
	}
	virtual void Tick() override
	{
		VOXEL_FUNCTION_COUNTER();

		FQueuedStat QueuedStat;
		while (Queue.Dequeue(QueuedStat))
		{
			for (const FVoxelCallstack* Callstack = QueuedStat.Callstack.Get(); Callstack; Callstack = Callstack->GetParent())
			{
				UEdGraphNode* GraphNode = Callstack->GetNodeRef().GetGraphNode_EditorOnly();
				if (!GraphNode)
				{
					continue;
				}

				NodeToTime.FindOrAdd(GraphNode) += QueuedStat.Time;
			}
		}
	}
	//~ End FVoxelSingleton Interface

	//~ Begin IVoxelNodeStatProvider Interface
	virtual void ClearStats() override
	{
		NodeToTime.Empty();
	}
	virtual bool IsEnabled(const UVoxelGraph& Graph) override
	{
		return GVoxelEnableNodeStats;
	}
	virtual FName GetIconStyleSetName() override
	{
		return STATIC_FNAME("EditorStyle");
	}
	virtual FName GetIconName() override
	{
		return STATIC_FNAME("GraphEditor.Timeline_16x");
	}
	virtual FText GetNodeToolTip(const UEdGraphNode& Node) override
	{
		const double* Time = NodeToTime.Find(&Node);
		if (!Time)
		{
			return {};
		}

		return FText::Format(
			INVTEXT("Total execution time of this node & all its downstream nodes: {0}"),
			FVoxelUtilities::ConvertToTimeText(*Time, 9));
	}
	virtual FText GetNodeText(const UEdGraphNode& Node) override
	{
		const double* Time = NodeToTime.Find(&Node);
		if (!Time)
		{
			return {};
		}

		return FText::Format(
			INVTEXT("Total Exec Time: {0}"),
			FVoxelUtilities::ConvertToTimeText(*Time));
	}
	//~ End IVoxelNodeStatProvider Interface
};
FVoxelExecNodeStatManager* GVoxelExecNodeStatManager = new FVoxelExecNodeStatManager();
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTaskGroupScope::~FVoxelTaskGroupScope()
{
	if (!Group)
	{
		return;
	}

	Group->RuntimeInfo->NumActiveTasks.Decrement();

	FPlatformTLS::SetTlsValue(GVoxelTaskGroupTLS, PreviousTLS);

#if WITH_EDITOR
	if (!GVoxelEnableNodeStats)
	{
		return;
	}

	const double Time = FPlatformTime::Seconds() - StartTime;
	GVoxelExecNodeStatManager->Queue.Enqueue(
	{
		Group->TerminalGraphInstance->Callstack,
		Time
	});
#endif
}

bool FVoxelTaskGroupScope::Initialize(FVoxelTaskGroup& NewGroup)
{
	if (NewGroup.RuntimeInfo->bDestroyStarted.Get())
	{
		return false;
	}

	NewGroup.RuntimeInfo->NumActiveTasks.Increment();

	if (NewGroup.RuntimeInfo->bDestroyStarted.Get())
	{
		Group->RuntimeInfo->NumActiveTasks.Decrement();
		return false;
	}

	Group = NewGroup.AsShared();

	PreviousTLS = FPlatformTLS::GetTlsValue(GVoxelTaskGroupTLS);
	FPlatformTLS::SetTlsValue(GVoxelTaskGroupTLS, &NewGroup);

	return true;
}