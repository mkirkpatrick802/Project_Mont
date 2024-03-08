// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelArrayNodes.generated.h"

// Get the number of items in an array
USTRUCT(Category = "Array", meta = (DisplayName = "Length", CompactNodeTitle = "LENGTH", Keywords = "num size count"))
struct VOXELGRAPHCORE_API FVoxelNode_ArrayLength : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	VOXEL_INPUT_PIN(FVoxelWildcardBuffer, Values, nullptr, ArrayPin);
	VOXEL_OUTPUT_PIN(int32, Result);

	//~ Begin FVoxelNode Interface
#if WITH_EDITOR
	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
#endif
	//~ End FVoxelNode Interface
};

// Given an array and an index, returns the item in the array at that index
USTRUCT(Category = "Array", meta = (DisplayName = "Get", CompactNodeTitle = "GET", Keywords = "item array"))
struct VOXELGRAPHCORE_API FVoxelNode_ArrayGetItem : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	VOXEL_INPUT_PIN(FVoxelWildcardBuffer, Values, nullptr, ArrayPin);
	VOXEL_TEMPLATE_INPUT_PIN(int32, Index, 0);
	VOXEL_TEMPLATE_OUTPUT_PIN(FVoxelWildcard, Result);

	//~ Begin FVoxelNode Interface
#if WITH_EDITOR
	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
#endif
	//~ End FVoxelNode Interface
};

// Create an array from a series of items
USTRUCT(Category = "Array", meta = (NodeIcon = "MakeArray", NodeIconColor = "White"))
struct VOXELGRAPHCORE_API FVoxelNode_MakeArray : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	VOXEL_VARIADIC_INPUT_PIN(FVoxelWildcard, Item, nullptr, 1);
	VOXEL_OUTPUT_PIN(FVoxelWildcardBuffer, Result, ArrayPin);

	//~ Begin FVoxelNode Interface
#if WITH_EDITOR
	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
#endif

	virtual bool IsPureNode() const override
	{
		return true;
	}
	//~ End FVoxelNode Interface

public:
#if WITH_EDITOR
	class FDefinition : public Super::FDefinition
	{
	public:
		GENERATED_VOXEL_NODE_DEFINITION_BODY(FVoxelNode_MakeArray);

		virtual FName Variadic_AddPinTo(FName VariadicPinName) override;
	};
#endif
};

// Add series of items to array
USTRUCT(Category = "Array")
struct VOXELGRAPHCORE_API FVoxelNode_AddToArray : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	VOXEL_INPUT_PIN(FVoxelWildcardBuffer, Array, nullptr, ArrayPin);
	VOXEL_VARIADIC_INPUT_PIN(FVoxelWildcard, Item, nullptr, 1);
	VOXEL_OUTPUT_PIN(FVoxelWildcardBuffer, Result, ArrayPin);

	//~ Begin FVoxelNode Interface
#if WITH_EDITOR
	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
#endif

	virtual bool IsPureNode() const override
	{
		return true;
	}
	//~ End FVoxelNode Interface

public:
#if WITH_EDITOR
	class FDefinition : public Super::FDefinition
	{
	public:
		GENERATED_VOXEL_NODE_DEFINITION_BODY(FVoxelNode_AddToArray);

		virtual FName Variadic_AddPinTo(FName VariadicPinName) override;
	};
#endif
};

// Append array with other array
USTRUCT(Category = "Array")
struct VOXELGRAPHCORE_API FVoxelNode_AppendArray : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	VOXEL_INPUT_PIN(FVoxelWildcardBuffer, A, nullptr, ArrayPin);
	VOXEL_INPUT_PIN(FVoxelWildcardBuffer, B, nullptr, ArrayPin);
	VOXEL_OUTPUT_PIN(FVoxelWildcardBuffer, Result, ArrayPin);

	//~ Begin FVoxelNode Interface
#if WITH_EDITOR
	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
#endif

	virtual bool IsPureNode() const override
	{
		return true;
	}
	//~ End FVoxelNode Interface
};