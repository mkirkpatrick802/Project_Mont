// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelISPCNodeHelpers.isph"
#include "VoxelNoiseNodesImpl.isph"

export void VoxelNode_VoxelNode_CellularNoise2D(const FVoxelBuffer* uniform Buffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float2 Position = MakeFloat2(LoadFloat(Buffers[0], Index), LoadFloat(Buffers[1], Index));
		const float Amplitude = LoadFloat(Buffers[2], Index);
		const float FeatureScale = LoadFloat(Buffers[3], Index);
		const float Jitter = LoadFloat(Buffers[4], Index);
		const int32 Seed = LoadInt32(Buffers[5], Index);
		float Value;

		Value = GetCellularNoise2D(Seed, Position / FeatureScale, Jitter) * Amplitude;

		StoreFloat(Buffers[6], Index, Value);
	}
}

export void VoxelNode_VoxelNode_CellularNoise3D(const FVoxelBuffer* uniform Buffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float3 Position = MakeFloat3(LoadFloat(Buffers[0], Index), LoadFloat(Buffers[1], Index), LoadFloat(Buffers[2], Index));
		const float Amplitude = LoadFloat(Buffers[3], Index);
		const float FeatureScale = LoadFloat(Buffers[4], Index);
		const float Jitter = LoadFloat(Buffers[5], Index);
		const int32 Seed = LoadInt32(Buffers[6], Index);
		float Value;

		Value = GetCellularNoise3D(Seed, Position / FeatureScale, Jitter) * Amplitude;

		StoreFloat(Buffers[7], Index, Value);
	}
}

export void VoxelNode_VoxelNode_MixSeeds(const FVoxelBuffer* uniform Buffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const int32 A = LoadInt32(Buffers[0], Index);
		const int32 B = LoadInt32(Buffers[1], Index);
		int32 ReturnValue;

		ReturnValue = MurmurHash32(MurmurHash32(A) ^ B);

		StoreInt32(Buffers[2], Index, ReturnValue);
	}
}

export void VoxelNode_VoxelNode_PerlinNoise2D(const FVoxelBuffer* uniform Buffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float2 Position = MakeFloat2(LoadFloat(Buffers[0], Index), LoadFloat(Buffers[1], Index));
		const float Amplitude = LoadFloat(Buffers[2], Index);
		const float FeatureScale = LoadFloat(Buffers[3], Index);
		const int32 Seed = LoadInt32(Buffers[4], Index);
		float Value;

		Value = GetPerlin2D(Seed, Position / FeatureScale) * Amplitude;

		StoreFloat(Buffers[5], Index, Value);
	}
}

export void VoxelNode_VoxelNode_PerlinNoise3D(const FVoxelBuffer* uniform Buffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float3 Position = MakeFloat3(LoadFloat(Buffers[0], Index), LoadFloat(Buffers[1], Index), LoadFloat(Buffers[2], Index));
		const float Amplitude = LoadFloat(Buffers[3], Index);
		const float FeatureScale = LoadFloat(Buffers[4], Index);
		const int32 Seed = LoadInt32(Buffers[5], Index);
		float Value;

		Value = GetPerlin3D(Seed, Position / FeatureScale) * Amplitude;

		StoreFloat(Buffers[6], Index, Value);
	}
}

export void VoxelNode_VoxelNode_TrueDistanceCellularNoise2D(const FVoxelBuffer* uniform Buffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float2 Position = MakeFloat2(LoadFloat(Buffers[0], Index), LoadFloat(Buffers[1], Index));
		const float Amplitude = LoadFloat(Buffers[2], Index);
		const float FeatureScale = LoadFloat(Buffers[3], Index);
		const float Jitter = LoadFloat(Buffers[4], Index);
		const int32 Seed = LoadInt32(Buffers[5], Index);
		float Value;
		float2 CellPosition;

		Value = GetTrueDistanceCellularNoise2D(Seed, Position / FeatureScale, Jitter, &CellPosition) * Amplitude;
		CellPosition = CellPosition * FeatureScale;

		StoreFloat(Buffers[6], Index, Value);
		StoreFloat(Buffers[7], Index, CellPosition.x);
		StoreFloat(Buffers[8], Index, CellPosition.y);
	}
}

export void VoxelNode_VoxelNode_TrueDistanceCellularNoise3D(const FVoxelBuffer* uniform Buffers, const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const float3 Position = MakeFloat3(LoadFloat(Buffers[0], Index), LoadFloat(Buffers[1], Index), LoadFloat(Buffers[2], Index));
		const float Amplitude = LoadFloat(Buffers[3], Index);
		const float FeatureScale = LoadFloat(Buffers[4], Index);
		const float Jitter = LoadFloat(Buffers[5], Index);
		const int32 Seed = LoadInt32(Buffers[6], Index);
		float Value;
		float3 CellPosition;

		Value = GetTrueDistanceCellularNoise3D(Seed, Position / FeatureScale, Jitter, &CellPosition) * Amplitude;
		CellPosition = CellPosition * FeatureScale;

		StoreFloat(Buffers[7], Index, Value);
		StoreFloat(Buffers[8], Index, CellPosition.x);
		StoreFloat(Buffers[9], Index, CellPosition.y);
		StoreFloat(Buffers[10], Index, CellPosition.z);
	}
}