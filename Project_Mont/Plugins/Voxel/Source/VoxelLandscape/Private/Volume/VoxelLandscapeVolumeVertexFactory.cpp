// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Volume/VoxelLandscapeVolumeVertexFactory.h"
#include "MaterialDomain.h"
#include "RHIStaticStates.h"
#include "GlobalRenderResources.h"
#include "MeshMaterialShader.h"
#include "MeshDrawShaderBindings.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelLandscapeVolumeVertexFactoryBase);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelLandscapeVolumeVertexFactoryShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
#define BIND(Name) Name.Bind(ParameterMap, TEXT(#Name))
	BIND(ScaledVoxelSize);
	BIND(RawVoxelSize);
	BIND(TextureSampler);

	BIND(Normal_Texture);
	BIND(Normal_Texture_Size);
	BIND(Normal_Texture_InvSize);
	BIND(Normal_TextureSize);
	BIND(Normal_TextureIndex);

	BIND(Material_TextureA);
	BIND(Material_TextureB);
	BIND(Material_Texture_Size);
	BIND(Material_Texture_InvSize);
	BIND(Material_TextureSize);
	BIND(Material_TextureIndex);
#undef BIND
}

void FVoxelLandscapeVolumeVertexFactoryShaderParameters::GetElementShaderBindings(
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
	const FVoxelLandscapeVolumeVertexFactoryBase& VoxelVertexFactory = static_cast<const FVoxelLandscapeVolumeVertexFactoryBase&>(*VertexFactory);

	ShaderBindings.Add(ScaledVoxelSize, float(VoxelVertexFactory.ScaledVoxelSize));
	ShaderBindings.Add(RawVoxelSize, float(VoxelVertexFactory.RawVoxelSize));

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	if (VoxelVertexFactory.Normal_Texture)
	{
		ShaderBindings.AddTexture(
			Normal_Texture,
			TextureSampler,
			TStaticSamplerState<SF_Bilinear>::GetRHI(),
			VoxelVertexFactory.Normal_Texture);

		ShaderBindings.Add(Normal_Texture_Size, float(VoxelVertexFactory.Normal_Texture->GetSizeX()));
		ShaderBindings.Add(Normal_Texture_InvSize, float(1. / VoxelVertexFactory.Normal_Texture->GetSizeX()));
	}
	else
	{
		ShaderBindings.AddTexture(
			Normal_Texture,
			TextureSampler,
			TStaticSamplerState<SF_Bilinear>::GetRHI(),
			GBlackTexture->TextureRHI);

		ShaderBindings.Add(Normal_Texture_Size, 0.f);
		ShaderBindings.Add(Normal_Texture_InvSize, 0.f);
	}

	ShaderBindings.Add(Normal_TextureSize, VoxelVertexFactory.Normal_TextureSize);
	ShaderBindings.Add(Normal_TextureIndex, VoxelVertexFactory.Normal_TextureIndex);

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	if (VoxelVertexFactory.Material_TextureA &&
		VoxelVertexFactory.Material_TextureB)
	{
		ensure(VoxelVertexFactory.Material_TextureA->GetSizeX() == VoxelVertexFactory.Material_TextureB->GetSizeX());

		ShaderBindings.AddTexture(
			Material_TextureA,
			TextureSampler,
			TStaticSamplerState<SF_Bilinear>::GetRHI(),
			VoxelVertexFactory.Material_TextureA);

		ShaderBindings.AddTexture(
			Material_TextureB,
			TextureSampler,
			TStaticSamplerState<SF_Bilinear>::GetRHI(),
			VoxelVertexFactory.Material_TextureB);

		ShaderBindings.Add(Normal_Texture_Size, float(VoxelVertexFactory.Material_TextureA->GetSizeX()));
		ShaderBindings.Add(Normal_Texture_InvSize, float(1. / VoxelVertexFactory.Material_TextureA->GetSizeX()));
	}
	else
	{
		ensure(!VoxelVertexFactory.Material_TextureA);
		ensure(!VoxelVertexFactory.Material_TextureB);

		ShaderBindings.AddTexture(
			Material_TextureA,
			TextureSampler,
			TStaticSamplerState<SF_Bilinear>::GetRHI(),
			GBlackTexture->TextureRHI);

		ShaderBindings.AddTexture(
			Material_TextureB,
			TextureSampler,
			TStaticSamplerState<SF_Bilinear>::GetRHI(),
			GBlackTexture->TextureRHI);

		ShaderBindings.Add(Normal_Texture_Size, 0.f);
		ShaderBindings.Add(Normal_Texture_InvSize, 0.f);
	}

	ShaderBindings.Add(Material_TextureSize, VoxelVertexFactory.Material_TextureSize);
	ShaderBindings.Add(Material_TextureIndex, VoxelVertexFactory.Material_TextureIndex);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelLandscapeVolumeVertexFactoryBase::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
{
	if (Parameters.MaterialParameters.bIsSpecialEngineMaterial)
	{
		return true;
	}

	return
		IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5) &&
		Parameters.MaterialParameters.MaterialDomain == MD_Surface;
}

void FVoxelLandscapeVolumeVertexFactoryBase::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_PRIMITIVE_SCENE_DATA"),
		UseGPUScene(
			Parameters.Platform,
			GetMaxSupportedFeatureLevel(Parameters.Platform)));

	OutEnvironment.SetDefine(TEXT("VOXEL_ENGINE_VERSION"), VOXEL_ENGINE_VERSION);
	OutEnvironment.SetDefine(TEXT("VOXEL_MARCHING_CUBE_VERTEX_FACTORY"), TEXT("1"));

	if (Parameters.VertexFactoryType == &FVoxelLandscapeVolumeVertexFactory_NoVertexNormals::StaticType)
	{
		OutEnvironment.SetDefine(TEXT("WITH_VERTEX_NORMALS"), TEXT("0"));
	}
	else
	{
		ensure(Parameters.VertexFactoryType == &FVoxelLandscapeVolumeVertexFactory_WithVertexNormals::StaticType);
		OutEnvironment.SetDefine(TEXT("WITH_VERTEX_NORMALS"), TEXT("1"));
	}
}

void FVoxelLandscapeVolumeVertexFactoryBase::GetPSOPrecacheVertexFetchElements(
	const EVertexInputStreamType VertexInputStreamType,
	FVertexDeclarationElementList& Elements,
	const bool bHasVertexNormals)
{
	Elements.Add(FVertexElement(0, 0, VET_Float3, 0, 0, false));
	Elements.Add(FVertexElement(1, 0, VET_UInt, 2, 0, true));

	if (bHasVertexNormals)
	{
		Elements.Add(FVertexElement(2, 0, VET_Float3, 3, 0, false));
	}
}

void FVoxelLandscapeVolumeVertexFactoryBase::InitRHI(UE_503_ONLY(FRHICommandListBase& RHICmdList))
{
	FVertexFactory::InitRHI(UE_503_ONLY(RHICmdList));

	const auto AddDeclaration = [&](const EVertexInputStreamType Type)
	{
		FVertexDeclarationElementList Elements;
		Elements.Add(AccessStreamComponent(PositionAndPrimitiveDataComponent, 0, Type));

		AddPrimitiveIdStreamElement(Type, Elements, 1, 0xFF);

		if (GetType() == &FVoxelLandscapeVolumeVertexFactory_WithVertexNormals::StaticType)
		{
			Elements.Add(AccessStreamComponent(VertexNormalComponent, 2, Type));
		}

		InitDeclaration(Elements, Type);
	};

	AddDeclaration(EVertexInputStreamType::Default);
	AddDeclaration(EVertexInputStreamType::PositionOnly);
	AddDeclaration(EVertexInputStreamType::PositionAndNormalOnly);
}

void FVoxelLandscapeVolumeVertexFactoryBase::ReleaseRHI()
{
	FVertexFactory::ReleaseRHI();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_TYPE_LAYOUT(FVoxelLandscapeVolumeVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelLandscapeVolumeVertexFactory_NoVertexNormals, SF_Pixel, FVoxelLandscapeVolumeVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelLandscapeVolumeVertexFactory_NoVertexNormals, SF_Vertex, FVoxelLandscapeVolumeVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelLandscapeVolumeVertexFactory_WithVertexNormals, SF_Pixel, FVoxelLandscapeVolumeVertexFactoryShaderParameters);
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FVoxelLandscapeVolumeVertexFactory_WithVertexNormals, SF_Vertex, FVoxelLandscapeVolumeVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_TYPE(FVoxelLandscapeVolumeVertexFactory_NoVertexNormals, "/Plugin/Voxel/VoxelLandscapeVolumeVertexFactory.ush",
	EVertexFactoryFlags::UsedWithMaterials |
	EVertexFactoryFlags::SupportsDynamicLighting |
	EVertexFactoryFlags::SupportsPositionOnly |
	EVertexFactoryFlags::SupportsCachingMeshDrawCommands |
	EVertexFactoryFlags::SupportsPrimitiveIdStream |
	EVertexFactoryFlags::SupportsPSOPrecaching);

IMPLEMENT_VERTEX_FACTORY_TYPE(FVoxelLandscapeVolumeVertexFactory_WithVertexNormals, "/Plugin/Voxel/VoxelLandscapeVolumeVertexFactory.ush",
	EVertexFactoryFlags::UsedWithMaterials |
	EVertexFactoryFlags::SupportsDynamicLighting |
	EVertexFactoryFlags::SupportsPositionOnly |
	EVertexFactoryFlags::SupportsCachingMeshDrawCommands |
	EVertexFactoryFlags::SupportsPrimitiveIdStream |
	EVertexFactoryFlags::SupportsPSOPrecaching)