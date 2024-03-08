// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelPointNodesStats.h"
#include "VoxelNodeStats.h"
#include "EdGraph/EdGraphNode.h"

#if WITH_EDITOR
class FVoxelNodePointFilterStatManager
	: public FVoxelSingleton
	, public IVoxelNodeStatProvider
{
public:
	struct FQueuedStat
	{
		FVoxelGraphNodeRef NodeRef;
		int64 Count = 0;
		int64 FilteredCount = 0;
	};
	TQueue<FQueuedStat, EQueueMode::Mpsc> Queue;

	struct FStats
	{
		int64 NumPoints = 0;
		int64 NumFilteredPoints = 0;
	};
	TVoxelMap<TWeakObjectPtr<const UEdGraphNode>, FStats> NodeToStats;

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
			UEdGraphNode* GraphNode = QueuedStat.NodeRef.GetGraphNode_EditorOnly();
			if (!ensure(GraphNode))
			{
				continue;
			}

			FStats& Stats = NodeToStats.FindOrAdd(GraphNode);
			Stats.NumPoints += QueuedStat.Count;
			Stats.NumFilteredPoints += QueuedStat.FilteredCount;
		}
	}
	//~ End FVoxelSingleton Interface

	//~ Begin IVoxelNodeStatProvider Interface
	virtual void ClearStats() override
	{
		NodeToStats.Empty();
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
		return "FoliageEditMode.Filter";
	}
	virtual FText GetNodeToolTip(const UEdGraphNode& Node) override
	{
		static FNumberFormattingOptions FormattingOptions = FNumberFormattingOptions().SetMinimumFractionalDigits(2).SetMaximumFractionalDigits(2);

		const FStats* Stats = NodeToStats.Find(&Node);
		if (!Stats)
		{
			return {};
		}

		return FText::Format(INVTEXT("Filtered {0} of {1} elements, {2}% filtered"),
			FVoxelUtilities::ConvertToNumberText(Stats->NumFilteredPoints),
			FVoxelUtilities::ConvertToNumberText(Stats->NumPoints),
			FText::AsNumber((double(Stats->NumFilteredPoints) / Stats->NumPoints) * 100.f, &FormattingOptions));
	}
	virtual FText GetNodeText(const UEdGraphNode& Node) override
	{
		static FNumberFormattingOptions FormattingOptions = FNumberFormattingOptions().SetMinimumFractionalDigits(2).SetMaximumFractionalDigits(2);

		const FStats* Stats = NodeToStats.Find(&Node);
		if (!Stats)
		{
			return {};
		}

		return FText::Format(INVTEXT("{0} - {1} = {2} | {3}%"),
			FVoxelUtilities::ConvertToNumberText(Stats->NumPoints),
			FVoxelUtilities::ConvertToNumberText(Stats->NumFilteredPoints),
			FVoxelUtilities::ConvertToNumberText(Stats->NumPoints - Stats->NumFilteredPoints),
			FText::AsNumber((double(Stats->NumPoints - Stats->NumFilteredPoints) / Stats->NumPoints) * 100.f, &FormattingOptions));
	}
	//~ End IVoxelNodeStatProvider Interface
};
FVoxelNodePointFilterStatManager* GVoxelNodePointFilterStatManager = new FVoxelNodePointFilterStatManager();
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelPointFilterStats::RecordNodeStats(const IVoxelNodeInterface& Node, const int64 Count, const int64 RemainingCount)
{
#if WITH_EDITOR
	if (!GVoxelEnableNodeStats)
	{
		return;
	}

	GVoxelNodePointFilterStatManager->Queue.Enqueue(
	{
		Node.GetNodeRef(),
		Count,
		Count - RemainingCount
	});
#endif
}