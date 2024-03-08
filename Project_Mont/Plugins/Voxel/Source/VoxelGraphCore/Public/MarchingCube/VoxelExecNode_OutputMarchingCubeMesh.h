// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelSurface.h"
#include "VoxelExecNode.h"
#include "VoxelChunkSpawner.h"
#include "VoxelDetailTexture.h"
#include "VoxelPhysicalMaterial.h"
#include "Material/VoxelMaterial.h"
#include "VoxelMarchingCubeMeshComponentSettings.h"
#include "VoxelExecNode_OutputMarchingCubeMesh.generated.h"

class FVoxelMarchingCubeMesh;
class FVoxelMarchingCubeCollider;

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelMarchingCubeExecNodeMesh
{
	GENERATED_BODY()

	TSharedPtr<FVoxelMarchingCubeMesh> Mesh;
	TSharedPtr<const FVoxelMarchingCubeMeshComponentSettings> MeshSettings;
	TSharedPtr<const FVoxelMarchingCubeCollider> Collider;
	TSharedPtr<const FBodyInstance> BodyInstance;
};

// This node is entirely disabled on dedicated servers
USTRUCT()
struct VOXELGRAPHCORE_API FVoxelExecNode_OutputMarchingCubeMesh : public FVoxelExecNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	// Defines where to spawn chunks & at what LOD
	// If not set will default to a Screen Size Chunk Spawner
	VOXEL_INPUT_PIN(FVoxelChunkSpawner, ChunkSpawner, nullptr, ConstantPin, OptionalPin);
	// The voxel size to use: distance in cm between 2 vertices when rendering
	VOXEL_INPUT_PIN(int32, VoxelSize, 100, ConstantPin);

	VOXEL_INPUT_PIN(FVoxelSurface, Surface, nullptr, VirtualPin);

	// Collision presets to use on the rendered chunks
	// If set to NoCollision will not compute collision at all
	// If your game is multiplayer or has NPCs you need to set this to NoCollision and setup invoker-based collision instead
	// https://docs.voxelplugin.com/basics/navmesh-and-collision
	VOXEL_INPUT_PIN(FBodyInstance, BodyInstance, nullptr, VirtualPin);
	VOXEL_INPUT_PIN(FVoxelPhysicalMaterialBuffer, PhysicalMaterial, nullptr, VirtualPin);

	// Material override, automatically set based on the first material definition being used if null
	VOXEL_INPUT_PIN(FVoxelMaterial, Material, nullptr, VirtualPin, AdvancedDisplay);
	// Mesh settings, used to tune mesh component settings like CastShadow, ReceiveDecals...
	VOXEL_INPUT_PIN(FVoxelMarchingCubeMeshComponentSettings, MeshSettings, nullptr, VirtualPin, AdvancedDisplay);
	VOXEL_INPUT_PIN(FVoxelMaterialDetailTextureRef, MaterialDetailTexture, "/Voxel/Default/DefaultMaterialTexture.DefaultMaterialTexture", VirtualPin, AdvancedDisplay);
	VOXEL_INPUT_PIN(FVoxelNormalDetailTextureRef, NormalDetailTexture, "/Voxel/Default/DefaultNormalTexture.DefaultNormalTexture", VirtualPin, AdvancedDisplay);
	// The node will query the distance at the chunk corners (slightly offset inside) to determine if the chunk needs to be rendered at all
	// This value controls by how much to inflate the distance used by these distance checks
	// If you're getting holes in your voxel mesh, try increasing this a bit (typically, setting to 2 or 3)
	// Don't increase too much, will add a lot of additional cost!
	VOXEL_INPUT_PIN(float, DistanceChecksTolerance, 1.f, VirtualPin, AdvancedDisplay);
	// If true, will try to make transitions perfect by querying the right LOD for border values
	// More expensive! Only use if you see holes between LODs
	VOXEL_INPUT_PIN(bool, PerfectTransitions, false, VirtualPin, AdvancedDisplay);
	VOXEL_INPUT_PIN(bool, GenerateDistanceFields, false, VirtualPin, AdvancedDisplay);
	VOXEL_INPUT_PIN(float, DistanceFieldBias, 0.f, VirtualPin, AdvancedDisplay);
	// Useful for reducing self shadowing from distance field methods when using world position offset to animate the mesh's vertices.
	VOXEL_INPUT_PIN(float, DistanceFieldSelfShadowBias, 0.f, VirtualPin, AdvancedDisplay);
	// Priority offset, added to the task distance from camera
	// Closest tasks are computed first, so set this to a very low value (eg, -1000000) if you want it to be computed first
	VOXEL_INPUT_PIN(double, PriorityOffset, 0, ConstantPin, AdvancedDisplay);

	TValue<FVoxelMarchingCubeExecNodeMesh> CreateMesh(
		const FVoxelQuery& InQuery,
		int32 VoxelSize,
		int32 ChunkSize,
		const FVoxelBox& Bounds) const;

	TValue<FVoxelSurface> GetSurface(const FVoxelQuery& Query) const;

	virtual TVoxelUniquePtr<FVoxelExecNodeRuntime> CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const override;
};