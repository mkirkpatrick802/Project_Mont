// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBuffer.h"
#include "VoxelMaterialBlending.generated.h"

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelMaterialBlending
{
	GENERATED_BODY()

	union
	{
		uint64 Raw;

		// Layout matches the GPU one
		// Will be split into 2 32 bit textures
		struct
		{
			uint32 AlphaA : 8;
			uint32 MaterialB : 12;
			uint32 MaterialA : 12;
			uint32 MaterialC : 16;
			uint32 AlphaC : 8;
			uint32 AlphaB : 8;
		};
	};

	FORCEINLINE FVoxelMaterialBlending()
	{
		Raw = 0;
	}

	static FVoxelMaterialBlending FromIndex(int32 Index);
	int32 GetBestIndex() const;
};
checkStatic(sizeof(FVoxelMaterialBlending) == sizeof(uint64));

DECLARE_VOXEL_TERMINAL_BUFFER(FVoxelMaterialBlendingBuffer, FVoxelMaterialBlending);

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelMaterialBlendingBuffer final : public FVoxelSimpleTerminalBuffer
{
	GENERATED_BODY()
	GENERATED_VOXEL_TERMINAL_BUFFER_BODY(FVoxelMaterialBlendingBuffer, FVoxelMaterialBlending);
};