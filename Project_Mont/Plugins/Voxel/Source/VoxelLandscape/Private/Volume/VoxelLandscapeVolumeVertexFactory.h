// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VertexFactory.h"

class FVoxelLandscapeVolumeVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
	DECLARE_TYPE_LAYOUT(FVoxelLandscapeVolumeVertexFactoryShaderParameters, NonVirtual);

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

	LAYOUT_FIELD(FShaderParameter, ScaledVoxelSize);
	LAYOUT_FIELD(FShaderParameter, RawVoxelSize);
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelLandscapeVolumeVertexFactoryBase : public FVertexFactory
{
public:
	int32 ScaledVoxelSize = 0;
	int32 RawVoxelSize = 0;

	FTextureRHIRef Normal_Texture;
	float Normal_TextureSize = 0;
	int32 Normal_TextureIndex = 0;

	FTextureRHIRef Material_TextureA;
	FTextureRHIRef Material_TextureB;
	float Material_TextureSize = 0;
	int32 Material_TextureIndex = 0;

	FVertexStreamComponent PositionAndPrimitiveDataComponent;
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelLandscapeVolumeVertexFactory_NoVertexNormals : public FVoxelLandscapeVolumeVertexFactoryBase
{
	DECLARE_VERTEX_FACTORY_TYPE(FVoxelLandscapeVolumeVertexFactory_NoVertexNormals);

public:
	using FVoxelLandscapeVolumeVertexFactoryBase::FVoxelLandscapeVolumeVertexFactoryBase;

	static void GetPSOPrecacheVertexFetchElements(
		const EVertexInputStreamType VertexInputStreamType,
		FVertexDeclarationElementList& Elements)
	{
		FVoxelLandscapeVolumeVertexFactoryBase::GetPSOPrecacheVertexFetchElements(VertexInputStreamType, Elements, false);
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelLandscapeVolumeVertexFactory_WithVertexNormals : public FVoxelLandscapeVolumeVertexFactoryBase
{
	DECLARE_VERTEX_FACTORY_TYPE(FVoxelLandscapeVolumeVertexFactory_WithVertexNormals);

public:
	using FVoxelLandscapeVolumeVertexFactoryBase::FVoxelLandscapeVolumeVertexFactoryBase;

	static void GetPSOPrecacheVertexFetchElements(
		const EVertexInputStreamType VertexInputStreamType,
		FVertexDeclarationElementList& Elements)
	{
		FVoxelLandscapeVolumeVertexFactoryBase::GetPSOPrecacheVertexFetchElements(VertexInputStreamType, Elements, true);
	}
};