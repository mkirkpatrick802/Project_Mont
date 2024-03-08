// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelNode.h"

struct FVoxelNodeLibrary
{
public:
	FVoxelNodeLibrary();

	static TConstVoxelArrayView<TSharedRef<const FVoxelNode>> GetNodes()
	{
		return Get().Nodes;
	}

	static TSharedPtr<const FVoxelNode> FindMakeNode(const FVoxelPinType& Type)
	{
		return Get().TypeToMakeNode.FindRef(Type);
	}
	static TSharedPtr<const FVoxelNode> FindBreakNode(const FVoxelPinType& Type)
	{
		return Get().TypeToBreakNode.FindRef(Type);
	}

	static TSharedPtr<const FVoxelNode> FindCastNode(const FVoxelPinType& From, const FVoxelPinType& To)
	{
		return Get().FromTypeAndToTypeToCastNode.FindRef({ From, To });
	}

	template<typename T>
	static TSharedPtr<const T> GetNodeInstance()
	{
		return StaticCastSharedPtr<const T>(Get().StructToNode.FindRef(T::StaticStruct()));
	}

private:
	TVoxelArray<TSharedRef<const FVoxelNode>> Nodes;
	TVoxelMap<FVoxelPinType, TSharedPtr<const FVoxelNode>> TypeToMakeNode;
	TVoxelMap < FVoxelPinType, TSharedPtr<const FVoxelNode>> TypeToBreakNode;
	TVoxelMap<TPair<FVoxelPinType, FVoxelPinType>, TSharedPtr<const FVoxelNode>> FromTypeAndToTypeToCastNode;
	TVoxelMap<const UScriptStruct*, TSharedPtr<const FVoxelNode>> StructToNode;

	static const FVoxelNodeLibrary& Get();
};