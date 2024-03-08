// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class FVoxelLandscapeState;

struct VOXELLANDSCAPE_API FVoxelLandscapeChunkKey
{
public:
	int32 LOD = 0;
	// ChunkPosition = ChunkKey * ChunkSize
	FIntVector ChunkKey = FIntVector(ForceInit);

	FORCEINLINE FVoxelIntBox GetChunkKeyBounds() const
	{
		return FVoxelIntBox(ChunkKey, ChunkKey + (1 << LOD));
	}

public:
	FVoxelBox2D GetQueriedBounds2D(const FVoxelLandscapeState& State) const;
	FVoxelBox GetQueriedBounds(const FVoxelLandscapeState& State)  const;

public:
	FORCEINLINE bool operator==(const FVoxelLandscapeChunkKey& Other) const
	{
		return
			LOD == Other.LOD &&
			ChunkKey == Other.ChunkKey;
	}
	FORCEINLINE friend uint32 GetTypeHash(const FVoxelLandscapeChunkKey& Key)
	{
		struct FKey
		{
			uint64 A;
			uint64 B;
		};
		return uint32(
			FVoxelUtilities::MurmurHash64(ReinterpretCastRef<FKey>(Key).A) ^
			FVoxelUtilities::MurmurHash64(ReinterpretCastRef<FKey>(Key).B));
	}
};
checkStatic(sizeof(FVoxelLandscapeChunkKey) == 16);

struct FVoxelLandscapeChunkKeyWithTransition
{
	FVoxelLandscapeChunkKey ChunkKey;
	uint8 TransitionMask = 0;
};