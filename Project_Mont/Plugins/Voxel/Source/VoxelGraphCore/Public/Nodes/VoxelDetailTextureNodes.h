// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelDetailTexture.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "Material/VoxelMaterial.h"
#include "Material/VoxelMaterialBlending.h"
#include "VoxelDetailTextureNodes.generated.h"

struct FVoxelMarchingCubeSurface;

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelDetailTextureQueryHelperImpl
{
	GENERATED_BODY()

public:
	void SetupQueryParameters(FVoxelQueryParameters& Parameters) const;

private:
	int32 TextureSize = 0;
	FVoxelVectorBuffer QueryPositions;
	TSharedPtr<const FVoxelMarchingCubeSurface> Surface;

	friend class FVoxelDetailTextureQueryHelper;
};

class VOXELGRAPHCORE_API FVoxelDetailTextureQueryHelper
{
public:
	const TSharedRef<const FVoxelMarchingCubeSurface> Surface;

	explicit FVoxelDetailTextureQueryHelper(const TSharedRef<const FVoxelMarchingCubeSurface>& Surface)
		: Surface(Surface)
	{
	}

	int32 NumCells() const;
	TVoxelFutureValue<FVoxelDetailTextureQueryHelperImpl> GetImpl(int32 TextureSize);

	int32 AddCellCoordinates(TVoxelArray<FVoxelDetailTextureCoordinate>&& CellCoordinates);
	TVoxelArray<uint8> BuildCellIndexToDirection() const;
	TVoxelArray<FVoxelDetailTextureCoordinate> BuildCellCoordinates() const;

private:
	mutable FVoxelCriticalSection CriticalSection;
	TVoxelMap<int32, TVoxelFutureValue<FVoxelDetailTextureQueryHelperImpl>> TextureSizeToImpl;
	TVoxelArray<TVoxelArray<FVoxelDetailTextureCoordinate>> AllCellCoordinates;

	FVoxelVectorBuffer ComputeQueryPositions(int32 TextureSize) const;
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelDetailTextureQueryParameter : public FVoxelQueryParameter
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_PARAMETER_BODY()

	TSharedPtr<FVoxelDetailTextureQueryHelper> Helper;
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelDetailTextureParameter : public FVoxelMaterialParameter
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	TOptional<FName> NameOverride;
	TSharedPtr<FVoxelDetailTexture> DetailTexture;
	TSharedPtr<const TVoxelComputeValue<FVoxelBuffer>> GetBuffer;

	template<typename T>
	void SetGetBuffer(const TSharedRef<const TVoxelComputeValue<T>>& GetBufferTyped)
	{
		GetBuffer = ReinterpretCastSharedRef<const TVoxelComputeValue<FVoxelBuffer>>(GetBufferTyped);
	}

	virtual TValue<FVoxelComputedMaterialParameter> Compute(const FVoxelQuery& Query) const final override;

protected:
	virtual void ComputeCell(
		int32 Index,
		int32 Pitch,
		int32 TextureSize,
		EPixelFormat PixelFormat,
		int32 TextureIndex,
		const FVoxelBuffer& Buffer,
		TVoxelArrayView<uint8> Data) const VOXEL_PURE_VIRTUAL();

	virtual TSharedPtr<FVoxelMaterialRef> GetMaterialOverride(const FVoxelBuffer& Buffer) const
	{
		return nullptr;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelFloatDetailTextureParameter : public FVoxelDetailTextureParameter
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	virtual void ComputeCell(
		int32 Index,
		int32 Pitch,
		int32 TextureSize,
		EPixelFormat PixelFormat,
		int32 TextureIndex,
		const FVoxelBuffer& Buffer,
		TVoxelArrayView<uint8> Data) const override;
};

USTRUCT(Category = "Material", meta = (Keywords = "scalar"))
struct VOXELGRAPHCORE_API FVoxelNode_MakeFloatDetailTextureParameter : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Float, nullptr);
	VOXEL_INPUT_PIN(FVoxelFloatDetailTextureRef, Texture, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelMaterialParameter, Parameter);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelColorDetailTextureParameter : public FVoxelDetailTextureParameter
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	virtual void ComputeCell(
		int32 Index,
		int32 Pitch,
		int32 TextureSize,
		EPixelFormat PixelFormat,
		int32 TextureIndex,
		const FVoxelBuffer& Buffer,
		TVoxelArrayView<uint8> Data) const override;
};

USTRUCT(Category = "Material")
struct VOXELGRAPHCORE_API FVoxelNode_MakeColorDetailTextureParameter : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelLinearColorBuffer, Color, nullptr);
	VOXEL_INPUT_PIN(FVoxelColorDetailTextureRef, Texture, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelMaterialParameter, Parameter);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelMaterialDetailTextureParameter : public FVoxelDetailTextureParameter
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	virtual void ComputeCell(
		int32 Index,
		int32 Pitch,
		int32 TextureSize,
		EPixelFormat PixelFormat,
		int32 TextureIndex,
		const FVoxelBuffer& Buffer,
		TVoxelArrayView<uint8> Data) const override;

	virtual TSharedPtr<FVoxelMaterialRef> GetMaterialOverride(const FVoxelBuffer& Buffer) const override;
};

USTRUCT(Category = "Material")
struct VOXELGRAPHCORE_API FVoxelNode_MakeMaterialDetailTextureParameter : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_CALL_PARAM(bool, bIsMain);

	VOXEL_INPUT_PIN(FVoxelMaterialBlendingBuffer, Material, nullptr);
	VOXEL_INPUT_PIN(FVoxelMaterialDetailTextureRef, Texture, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelMaterialParameter, Parameter);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelNormalDetailTextureParameter : public FVoxelDetailTextureParameter
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	virtual void ComputeCell(
		int32 Index,
		int32 Pitch,
		int32 TextureSize,
		EPixelFormat PixelFormat,
		int32 TextureIndex,
		const FVoxelBuffer& Buffer,
		TVoxelArrayView<uint8> Data) const override;
};

USTRUCT(Category = "Material")
struct VOXELGRAPHCORE_API FVoxelNode_MakeNormalDetailTextureParameter : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_CALL_PARAM(bool, bIsMain);

	VOXEL_INPUT_PIN(FVoxelVectorBuffer, Normal, nullptr);
	VOXEL_INPUT_PIN(FVoxelNormalDetailTextureRef, Texture, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelMaterialParameter, Parameter);
};