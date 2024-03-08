// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VertexFactory.h"
#include "VoxelDetailTexture.h"
#include "MarchingCube/VoxelMarchingCubeSurface.h"

struct FVoxelComputedMaterial;
class FCardRepresentationData;
class FDistanceFieldVolumeData;
class UVoxelMarchingCubeMeshComponent;

DECLARE_VOXEL_MEMORY_STAT(VOXELGRAPHCORE_API, STAT_VoxelMarchingCubeMeshMemory, "Voxel Marching Cube Mesh Memory");
DECLARE_VOXEL_MEMORY_STAT(VOXELGRAPHCORE_API, STAT_VoxelMarchingCubeMeshGpuMemory, "Voxel Marching Cube Mesh Memory (GPU)");

class FVoxelMarchingCubeVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
	DECLARE_TYPE_LAYOUT(FVoxelMarchingCubeVertexFactoryShaderParameters, NonVirtual);

public:
	void Bind(const FShaderParameterMap& ParameterMap);
	void GetElementShaderBindings(
		const FSceneInterface* Scene,
		const FSceneView* View,
		const FMeshMaterialShader* Shader,
		EVertexInputStreamType InputStreamType,
		ERHIFeatureLevel::Type FeatureLevel,
		const FVertexFactory* VertexFactory,
		const FMeshBatchElement& BatchElement,
		FMeshDrawSingleShaderBindings& ShaderBindings,
		FVertexInputStreamArray& VertexStreams) const;

	LAYOUT_FIELD(FShaderParameter, VoxelSize);
	LAYOUT_FIELD(FShaderParameter, NumCells);
	LAYOUT_FIELD(FShaderResourceParameter, CellTextureCoordinates);
	LAYOUT_FIELD(FShaderResourceParameter, TextureSampler);

	LAYOUT_FIELD(FShaderResourceParameter, Normal_Texture);
	LAYOUT_FIELD(FShaderParameter, Normal_Texture_Size);
	LAYOUT_FIELD(FShaderParameter, Normal_Texture_InvSize);
	LAYOUT_FIELD(FShaderParameter, Normal_TextureSize);
	LAYOUT_FIELD(FShaderParameter, Normal_TextureIndex);

	LAYOUT_FIELD(FShaderResourceParameter, Material_TextureA);
	LAYOUT_FIELD(FShaderResourceParameter, Material_TextureB);
	LAYOUT_FIELD(FShaderParameter, Material_Texture_Size);
	LAYOUT_FIELD(FShaderParameter, Material_Texture_InvSize);
	LAYOUT_FIELD(FShaderParameter, Material_TextureSize);
	LAYOUT_FIELD(FShaderParameter, Material_TextureIndex);
};

class FVoxelMarchingCubeVertexFactoryBase : public FVertexFactory
{
public:
	int32 VoxelSize = 0;
	int32 NumCells = 0;
	FShaderResourceViewRHIRef CellTextureCoordinates;

	FTextureRHIRef Normal_Texture;
	float Normal_Texture_Size = 1;
	float Normal_Texture_InvSize = 1;
	float Normal_TextureSize = 0;
	int32 Normal_TextureIndex = 0;

	FTextureRHIRef Material_TextureA;
	FTextureRHIRef Material_TextureB;
	float Material_Texture_Size = 1;
	float Material_Texture_InvSize = 1;
	float Material_TextureSize = 0;
	int32 Material_TextureIndex = 0;

	FVertexStreamComponent PositionComponent;
	FVertexStreamComponent PrimitiveDataComponent;
	FVertexStreamComponent VertexNormalComponent;

	using FVertexFactory::FVertexFactory;

	VOXEL_COUNT_INSTANCES();

	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
	static void GetPSOPrecacheVertexFetchElements(
		EVertexInputStreamType VertexInputStreamType,
		FVertexDeclarationElementList& Elements,
		bool bHasVertexNormals);

	virtual bool SupportsPositionOnlyStream() const override { return true; }
	virtual bool SupportsPositionAndNormalOnlyStream() const override { return true; }

	//~ Begin FRenderResource Interface
	virtual void InitRHI(UE_503_ONLY(FRHICommandListBase& RHICmdList)) override;
	virtual void ReleaseRHI() override;
	//~ End FRenderResource Interface
};

class FVoxelMarchingCubeVertexFactory_NoVertexNormals : public FVoxelMarchingCubeVertexFactoryBase
{
	DECLARE_VERTEX_FACTORY_TYPE(FVoxelMarchingCubeVertexFactory_NoVertexNormals);

public:
	using FVoxelMarchingCubeVertexFactoryBase::FVoxelMarchingCubeVertexFactoryBase;

	static void GetPSOPrecacheVertexFetchElements(
		const EVertexInputStreamType VertexInputStreamType,
		FVertexDeclarationElementList& Elements)
	{
		FVoxelMarchingCubeVertexFactoryBase::GetPSOPrecacheVertexFetchElements(VertexInputStreamType, Elements, false);
	}
};

class FVoxelMarchingCubeVertexFactory_WithVertexNormals : public FVoxelMarchingCubeVertexFactoryBase
{
	DECLARE_VERTEX_FACTORY_TYPE(FVoxelMarchingCubeVertexFactory_WithVertexNormals);

public:
	using FVoxelMarchingCubeVertexFactoryBase::FVoxelMarchingCubeVertexFactoryBase;

	static void GetPSOPrecacheVertexFetchElements(
		const EVertexInputStreamType VertexInputStreamType,
		FVertexDeclarationElementList& Elements)
	{
		FVoxelMarchingCubeVertexFactoryBase::GetPSOPrecacheVertexFetchElements(VertexInputStreamType, Elements, true);
	}
};

class FVoxelMarchingCubeMesh : public TSharedFromThis<FVoxelMarchingCubeMesh>
{
public:
	int32 LOD = 0;
	FVoxelBox Bounds;
	int32 VoxelSize = 0;
	int32 ChunkSize = 0;
	int32 NumCells = 0;
	int32 NumEdgeVertices = 0;
	TSharedPtr<const FVoxelComputedMaterial> ComputedMaterial;

	bool bHasVertexNormals = false;

	TVoxelArray<int32> Indices;
	TVoxelArray<FVector3f> Vertices;
	TVoxelArray<FVector3f> VertexNormals;
	TVoxelArray<int32> CellIndices;

	using FTransitionIndex = FVoxelMarchingCubeSurface::FTransitionIndex;
	using FTransitionVertex = FVoxelMarchingCubeSurface::FTransitionVertex;

	TVoxelStaticArray<TVoxelArray<FTransitionIndex>, 6> TransitionIndices;
	TVoxelStaticArray<TVoxelArray<FTransitionVertex>, 6> TransitionVertices;
	TVoxelStaticArray<TVoxelArray<int32>, 6> TransitionCellIndices;

	TVoxelArray<uint8> CellIndexToDirection;
	TVoxelArray<FVoxelDetailTextureCoordinate> CellTextureCoordinates;

	TSharedPtr<FCardRepresentationData> CardRepresentationData;
	TSharedPtr<FDistanceFieldVolumeData> DistanceFieldVolumeData;
	float SelfShadowBias = 0.f;

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelMarchingCubeMeshMemory);
	VOXEL_ALLOCATED_SIZE_GPU_TRACKER(STAT_VoxelMarchingCubeMeshGpuMemory);

	void SetTransitionMask_GameThread(uint8 NewTransitionMask);
	void SetTransitionMask_RenderThread(FRHICommandListBase& RHICmdList, uint8 NewTransitionMask);

	FVoxelBox GetBounds() const { return Bounds; }
	int64 GetAllocatedSize() const;
	int64 GetGpuAllocatedSize() const;
	TSharedRef<FVoxelMaterialRef> GetMaterial() const;

	void Initialize_GameThread(UVoxelMarchingCubeMeshComponent& InComponent);

	void InitializeIfNeeded_RenderThread(FRHICommandListBase& RHICmdList, ERHIFeatureLevel::Type FeatureLevel);
	void Destroy_RenderThread();

	static TSharedRef<FVoxelMarchingCubeMesh> New();

private:
	TWeakObjectPtr<UVoxelMarchingCubeMeshComponent> WeakComponent;

	bool bIsInitialized_RenderThread = false;
	bool bIsDestroyed_RenderThread = false;

	int32 NumIndicesToRender = 0;
	int32 NumVerticesToRender = 0;
	uint8 TransitionMask = 0;

	TSharedPtr<FIndexBuffer> IndicesBuffer;
	TSharedPtr<FVertexBuffer> VerticesBuffer;
	TSharedPtr<FVertexBuffer> PrimitivesDataBuffer;
	TSharedPtr<FVertexBuffer> VertexNormalsBuffer;

	TSharedPtr<FVertexBufferWithSRV> CellTextureCoordinatesBuffer;

	TSharedPtr<FVoxelMarchingCubeVertexFactoryBase> VertexFactory;
	TSharedPtr<FVoxelMaterialRef> Material;

	friend class FVoxelMarchingCubeMeshSceneProxy;
};