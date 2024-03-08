// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class UVoxelGraph;
struct IVoxelNodeInterface;

#if WITH_EDITOR
extern VOXELGRAPHCORE_API bool GVoxelEnableNodeStats;

struct IVoxelNodeStatProvider
{
	IVoxelNodeStatProvider() = default;
	virtual ~IVoxelNodeStatProvider() = default;

	virtual void ClearStats() = 0;
	virtual bool IsEnabled(const UVoxelGraph& Graph) = 0;
	virtual FName GetIconStyleSetName() = 0;
	virtual FName GetIconName() = 0;

	virtual FText GetNodeToolTip(const UEdGraphNode& Node) { return {}; }
	virtual FText GetPinToolTip(const UEdGraphPin& Pin) { return {}; }
	virtual FText GetNodeText(const UEdGraphNode& Node) { return {}; }
	virtual FText GetPinText(const UEdGraphPin& Pin) { return {}; }
};
extern VOXELGRAPHCORE_API TArray<IVoxelNodeStatProvider*> GVoxelNodeStatProviders;
#endif

class VOXELGRAPHCORE_API FVoxelNodeStatScope
{
public:
	FORCEINLINE FVoxelNodeStatScope(const IVoxelNodeInterface& InNode, const int64 InCount)
	{
#if WITH_EDITOR
		if (!GVoxelEnableNodeStats)
		{
			return;
		}

		Node = &InNode;
		Count = InCount;
		StartTime = FPlatformTime::Seconds();
#endif
	}
	FORCEINLINE ~FVoxelNodeStatScope()
	{
#if WITH_EDITOR
		if (IsEnabled())
		{
			RecordStats(FPlatformTime::Seconds() - StartTime);
		}
#endif
	}

	FORCEINLINE bool IsEnabled() const
	{
#if WITH_EDITOR
		return Node != nullptr;
#else
		return false;
#endif
	}
	FORCEINLINE void SetCount(const int32 NewCount)
	{
#if WITH_EDITOR
		Count = NewCount;
#endif
	}

private:
#if WITH_EDITOR
	const IVoxelNodeInterface* Node = nullptr;
	int64 Count = 0;
	double StartTime = 0;

	void RecordStats(double Duration) const;
#endif
};