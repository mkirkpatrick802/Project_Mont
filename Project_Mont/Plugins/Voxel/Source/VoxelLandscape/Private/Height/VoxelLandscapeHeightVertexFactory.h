// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VertexFactory.h"

class FVoxelLandscapeHeightVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
	DECLARE_TYPE_LAYOUT(FVoxelLandscapeHeightVertexFactoryShaderParameters, NonVirtual);

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

	LAYOUT_FIELD(FShaderParameter, ChunkSize);
	LAYOUT_FIELD(FShaderParameter, ScaledVoxelSize);
	LAYOUT_FIELD(FShaderParameter, RawVoxelSize);
	LAYOUT_FIELD(FShaderResourceParameter, Heights);
	LAYOUT_FIELD(FShaderResourceParameter, TextureSampler);
	LAYOUT_FIELD(FShaderResourceParameter, NormalTexture);
	LAYOUT_FIELD(FShaderParameter, NormalTextureSize);
};

class FVoxelLandscapeHeightVertexFactory : public FVertexFactory
{
public:
	DECLARE_VERTEX_FACTORY_TYPE(FVoxelLandscapeHeightVertexFactory);

	int32 ChunkSize = 0;
	int32 ScaledVoxelSize = 0;
	int32 RawVoxelSize = 0;
	FShaderResourceViewRHIRef Heights;
	FTextureRHIRef NormalTexture;

	using FVertexFactory::FVertexFactory;

	VOXEL_COUNT_INSTANCES();

	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);
	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);
	static void GetPSOPrecacheVertexFetchElements(
		EVertexInputStreamType VertexInputStreamType,
		FVertexDeclarationElementList& Elements);

	virtual bool SupportsPositionOnlyStream() const override { return true; }
	virtual bool SupportsPositionAndNormalOnlyStream() const override { return true; }

	//~ Begin FRenderResource Interface
	virtual void InitRHI(UE_503_ONLY(FRHICommandListBase& RHICmdList)) override;
	//~ End FRenderResource Interface
};