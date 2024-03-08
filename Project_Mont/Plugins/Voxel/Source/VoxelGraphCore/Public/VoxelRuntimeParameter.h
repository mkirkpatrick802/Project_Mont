// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntimeParameter.generated.h"

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelRuntimeParameter : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
};

class VOXELGRAPHCORE_API FVoxelRuntimeParameters
{
public:
	FVoxelRuntimeParameters() = default;

	void Add(const TSharedRef<const FVoxelRuntimeParameter>& RuntimeParameter)
	{
		StructToRuntimeParameter.Add_EnsureNew(RuntimeParameter->GetStruct(), RuntimeParameter);
	}
	template<typename T, typename = std::enable_if_t<TIsDerivedFrom<T, FVoxelRuntimeParameter>::Value>>
	T& Add()
	{
		const TSharedRef<T> Parameter = MakeVoxelShared<T>();
		this->Add(Parameter);
		return *Parameter;
	}

	template<typename T, typename = std::enable_if_t<TIsDerivedFrom<T, FVoxelRuntimeParameter>::Value>>
	bool Remove()
	{
		return StructToRuntimeParameter.Remove(StaticStructFast<T>()) != 0;
	}

	template<typename T, typename = std::enable_if_t<TIsDerivedFrom<T, FVoxelRuntimeParameter>::Value>>
	TSharedPtr<const T> Find() const
	{
		const TSharedPtr<const FVoxelRuntimeParameter> RuntimeParameter = StructToRuntimeParameter.FindRef(StaticStructFast<T>());
		checkVoxelSlow(!RuntimeParameter || RuntimeParameter->IsA<T>());
		return StaticCastSharedPtr<const T>(RuntimeParameter);
	}

private:
	TVoxelMap<UScriptStruct*, TSharedPtr<const FVoxelRuntimeParameter>> StructToRuntimeParameter;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelRuntimeParameter_DisableCollision : public FVoxelRuntimeParameter
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()
};