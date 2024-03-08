// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Height/VoxelLandscapeHeightVertexFactory.h"
#include "MaterialDomain.h"
#include "RHIStaticStates.h"
#include "MeshMaterialShader.h"
#include "MeshDrawShaderBindings.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelLandscapeHeightVertexFactory);

void FVoxelLandscapeHeightVertexFactoryShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
#define BIND(Name) Name.Bind(ParameterMap, TEXT(#Name))
	BIND(ChunkSize);
	BIND(ScaledVoxelSize);
	BIND(RawVoxelSize);
	BIND(Heights);
	BIND(TextureSampler);
	BIND(NormalTexture);
	BIND(NormalTextureSize);
#undef BIND
}

void FVoxelLandscapeHeightVertexFactoryShaderParameters::GetElementShaderBindings(
	const FSceneInterface* Scene,
	const FSceneView* View,
	const FMeshMaterialShader* Shader,
	const EVertexInputStreamType InputStreamType,
	ERHIFeatureLevel::Type FeatureLevel,
	const FVertexFactory* VertexFactory,
	const FMeshBatchElement& BatchElement,
	FMeshDrawSingleShaderBindings& ShaderBindings,
	FVertexInputStreamArray& VertexStreams) const
{
	const FVoxelLandscapeHeightVertexFactory& TypedVertexFactory = static_cast<const FVoxelLandscapeHeightVertexFactory&>(*VertexFactory);

	ShaderBindings.Add(ScaledVoxelSize, float(TypedVertexFactory.ScaledVoxelSize));
	ShaderBindings.Add(RawVoxelSize, float(TypedVertexFactory.RawVoxelSize));
	ShaderBindings.Add(ChunkSize, float(TypedVertexFactory.ChunkSize));

	ShaderBindings.Add(Heights, TypedVertexFactory.Heights);

	ShaderBindings.AddTexture(
		NormalTexture,
		TextureSampler,
		TStaticSamplerState<SF_Bilinear>::GetRHI(),
		TypedVertexFactory.NormalTexture);

	ShaderBindings.Add(NormalTextureSize, TypedVertexFactory.NormalTexture->GetSizeX());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelLandscapeHeightVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	if (Parameters.MaterialParameters.bIsSpecialEngineMaterial)
	{
		return true;
	}

	return
		IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
		Parameters.MaterialParameters.MaterialDomain == MD_Surface;
}

void FVoxelLandscapeHeightVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_PRIMITIVE_SCENE_DATA"),
		UseGPUScene(
			Parameters.Platform,
			GetMaxSupportedFeatureLevel(Parameters.Platform)));

	OutEnvironment.SetDefine(TEXT("VOXEL_ENGINE_VERSION"), VOXEL_ENGINE_VERSION);
	OutEnvironment.SetDefine(TEXT("VOXEL_LANDSCAPE_HEIGHT_VERTEX_FACTORY"), TEXT("1"));
}

void FVoxelLandscapeHeightVertexFactory::GetPSOPrecacheVertexFetchElements(
	const EVertexInputStreamType VertexInputStreamType,
	FVertexDeclarationElementList& Elements)
{
	Elements.Add(FVertexElement(0, 0, VET_UInt, 0, 0, true));
}

void FVoxelLandscapeHeightVertexFactory::InitRHI(UE_503_ONLY(FRHICommandListBase& RHICmdList))
{
	FVertexFactory::InitRHI(UE_503_ONLY(RHICmdList));

	const auto AddDeclaration = [&](const EVertexInputStreamType Type)
	{
		FVertexDeclarationElementList Elements;
		AddPrimitiveIdStreamElement(Type, Elements, 0, 0xFF);
		InitDeclaration(Elements, Type);
	};

	AddDeclaration(EVertexInputStreamType::Default);
	AddDeclaration(EVertexInputStreamType::PositionOnly);
	AddDeclaration(EVertexInputStreamType::PositionAndNormalOnly);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_TYPE_LAYOUT(FVoxelLandscapeHeightVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelLandscapeHeightVertexFactory, SF_Pixel, FVoxelLandscapeHeightVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelLandscapeHeightVertexFactory, SF_Vertex, FVoxelLandscapeHeightVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_TYPE(FVoxelLandscapeHeightVertexFactory, "/Plugin/Voxel/VoxelLandscapeHeightVertexFactory.ush",
	EVertexFactoryFlags::UsedWithMaterials |
	EVertexFactoryFlags::SupportsDynamicLighting |
	EVertexFactoryFlags::SupportsPositionOnly |
	EVertexFactoryFlags::SupportsCachingMeshDrawCommands |
	EVertexFactoryFlags::SupportsPrimitiveIdStream |
	EVertexFactoryFlags::SupportsPSOPrecaching);