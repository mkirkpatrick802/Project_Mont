// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBuffer.h"
#include "VoxelQueryParameter.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "VoxelSurface.generated.h"

struct VOXELGRAPHCORE_API FVoxelSurfaceAttributes
{
	static const FName Height;
	static const FName Distance;
	static const FName Material;

	static TSharedRef<const FVoxelBuffer> MakeDefault(
		const FVoxelPinType& InnerType,
		FName Attribute);
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelSurfaceQueryParameter : public FVoxelQueryParameter
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_PARAMETER_BODY()

public:
	TVoxelSet<FName> AttributesToCompute;

public:
	FORCEINLINE void Compute(const FName Attribute)
	{
		AttributesToCompute.Add(Attribute);
	}
	FORCEINLINE bool ShouldCompute(const FName Attribute) const
	{
		return AttributesToCompute.Contains(Attribute);
	}

public:
	FORCEINLINE void ComputeDistance()
	{
		Compute(FVoxelSurfaceAttributes::Distance);
	}
	FORCEINLINE bool ShouldComputeDistance() const
	{
		return ShouldCompute(FVoxelSurfaceAttributes::Distance);
	}
};

USTRUCT()
struct FVoxelSurfaceAttributeInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Config")
	FName Name = "Distance";

	UPROPERTY(EditAnywhere, Category = "Config", meta = (FilterTypes = AllBuffers))
	FVoxelPinType Type = FVoxelPinType::Make<float>().GetBufferType();
};

USTRUCT(meta = (NoBufferType))
struct VOXELGRAPHCORE_API FVoxelSurface : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FORCEINLINE const FVoxelBox& GetBounds() const
	{
		return Bounds;
	}
	FORCEINLINE const TVoxelMap<FName, TSharedPtr<const FVoxelBuffer>>& GetNameToAttribute() const
	{
		return NameToAttribute;
	}

	FORCEINLINE bool HasAttribute(const FName Attribute) const
	{
		return AttributesToCompute.Contains(Attribute);
	}
	FORCEINLINE bool HasDistance() const
	{
		return HasAttribute(FVoxelSurfaceAttributes::Distance);
	}

public:
	TSharedRef<const FVoxelBuffer> Get(
		const FVoxelPinType& InnerType,
		FName Attribute) const;

	template<typename InnerType, typename BufferType = typename TVoxelBufferType<InnerType>::Type>
	FORCEINLINE BufferType Get(const FName Attribute) const
	{
		return this->Get(FVoxelPinType::Make<InnerType>(), Attribute)->template AsChecked<BufferType>();
	}
	FORCEINLINE FVoxelFloatBuffer GetDistance() const
	{
		return Get<float>(FVoxelSurfaceAttributes::Distance);
	}

public:
	FORCEINLINE void SetBounds(const FVoxelBox& NewBounds)
	{
		Bounds = NewBounds;
	}
	FORCEINLINE void Set(const FName Attribute, const TSharedRef<const FVoxelBuffer>& Buffer)
	{
		NameToAttribute.FindOrAdd(Attribute) = Buffer;
	}
	FORCEINLINE void SetDistance(const FVoxelFloatBuffer& Buffer)
	{
		Set(FVoxelSurfaceAttributes::Distance, Buffer.MakeSharedCopy());
	}

private:
	FVoxelBox Bounds;
	TVoxelSet<FName> AttributesToCompute;
	TVoxelMap<FName, TSharedPtr<const FVoxelBuffer>> NameToAttribute;

	friend struct FVoxelNode_MakeSurface;
	friend struct FVoxelNode_GetSurfaceAttributes;
};