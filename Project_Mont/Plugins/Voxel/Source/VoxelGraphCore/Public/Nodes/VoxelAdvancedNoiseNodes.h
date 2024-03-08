// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "VoxelAdvancedNoiseNodes.generated.h"

UENUM(BlueprintType, DisplayName = "Advanced Noise Octave Type")
enum class EVoxelAdvancedNoiseOctaveType : uint8
{
	Default = 200,
	SmoothPerlin = 0 UMETA(ToolTip = "Perlin2D"),
	BillowyPerlin = 1 UMETA(ToolTip = "abs(Perlin2D) * 2 - 1"),
	RidgedPerlin = 2 UMETA(ToolTip = "(1 - abs(Perlin2D)) * 2 - 1"),

	SmoothCellular = 3 UMETA(ToolTip = "Cellular2D"),
	BillowyCellular = 4 UMETA(ToolTip = "abs(Cellular2D) * 2 - 1"),
	RidgedCellular = 5 UMETA(ToolTip = "(1 - abs(Cellular2D)) * 2 - 1"),
};

// Generates multi-octave, highly configurable height noise for easy procedural creation
USTRUCT(Category = "Noise")
struct VOXELGRAPHCORE_API FVoxelNode_AdvancedNoise2D : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	// Position at which to calculate output noise
	VOXEL_INPUT_PIN(FVoxelVector2DBuffer, Position, nullptr, NoDefault);
	// Height difference of the lowest and highest point of the noise's largest octave
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Amplitude, 10000.f);
	// Amount of space the noise will take to tile in the world, a divisor for position
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, FeatureScale, 100000.f);
	// A factor for how much smaller each octave's feature scale is compared to last octave's
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Lacunarity, 2.f);
	// A factor for how much smaller each octave's amplitude is compared to last octave's
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Gain, 0.5f);
	// Random offset applied to cell position when using cellular noise
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, CellularJitter, 0.9f);
	// Amount of layers this noise should have
	VOXEL_INPUT_PIN(int32, NumOctaves, 10);
	// Used to randomize the output noise
	VOXEL_INPUT_PIN(FVoxelSeed, Seed, nullptr);
	// Default octave type
	VOXEL_INPUT_PIN(EVoxelAdvancedNoiseOctaveType, DefaultOctaveType, EVoxelAdvancedNoiseOctaveType::SmoothPerlin, ShowInDetail);
	// Noise type to use for generating a given octave
	VOXEL_VARIADIC_INPUT_PIN(EVoxelAdvancedNoiseOctaveType, OctaveType, EVoxelAdvancedNoiseOctaveType::Default, 0, ShowInDetail);
	// Multiplier for the amplitude of a given octave
	VOXEL_VARIADIC_INPUT_PIN(FVoxelFloatBuffer, OctaveStrength, 1.f, 0, ShowInDetail);

	// Result of all octaves being added together
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Value);
};

// Generates multi-octave, highly configurable height noise for easy procedural creation
USTRUCT(Category = "Noise")
struct VOXELGRAPHCORE_API FVoxelNode_AdvancedNoise3D : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	// Position at which to calculate output noise
	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Position, nullptr, NoDefault);
	// Height difference of the lowest and highest point of the noise's largest octave
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Amplitude, 10000.f);
	// Amount of space the noise will take to tile in the world, a divisor for position
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, FeatureScale, 100000.f);
	// A factor for how much smaller each octave's feature scale is compared to last octave's
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Lacunarity, 2.f);
	// A factor for how much smaller each octave's amplitude is compared to last octave's
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Gain, 0.5f);
	// Random offset applied to cell position when using cellular noise
	VOXEL_INPUT_PIN(FVoxelFloatBuffer, CellularJitter, 0.9f);
	// Amount of layers this noise should have
	VOXEL_INPUT_PIN(int32, NumOctaves, 10);
	// Used to randomize the output noise
	VOXEL_INPUT_PIN(FVoxelSeed, Seed, nullptr);
	// Default octave type
	VOXEL_INPUT_PIN(EVoxelAdvancedNoiseOctaveType, DefaultOctaveType, EVoxelAdvancedNoiseOctaveType::SmoothPerlin, ShowInDetail);
	// Noise type to use for generating a given octave
	VOXEL_VARIADIC_INPUT_PIN(EVoxelAdvancedNoiseOctaveType, OctaveType, EVoxelAdvancedNoiseOctaveType::Default, 0, ShowInDetail);
	// Multiplier for the amplitude of a given octave
	VOXEL_VARIADIC_INPUT_PIN(FVoxelFloatBuffer, OctaveStrength, 1.f, 0, ShowInDetail);

	// Result of all octaves being added together
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Value);
};