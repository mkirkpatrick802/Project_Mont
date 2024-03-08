// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class FVoxelTaskGroup;

struct VOXELGRAPHCORE_API FVoxelTaskGroupScope
{
public:
	FVoxelTaskGroupScope() = default;
	~FVoxelTaskGroupScope();

	[[nodiscard]] bool Initialize(FVoxelTaskGroup& NewGroup);

	FORCEINLINE FVoxelTaskGroup& GetGroup() const
	{
		return *Group;
	}

private:
	TSharedPtr<FVoxelTaskGroup> Group;
	void* PreviousTLS = nullptr;
	double StartTime = FPlatformTime::Seconds();
};