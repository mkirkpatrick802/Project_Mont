// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFutureValue.h"
#include "VoxelGraphNodeRef.h"

struct FVoxelNode;

struct VOXELGRAPHCORE_API IVoxelNodeInterface
{
public:
	IVoxelNodeInterface() = default;
	virtual ~IVoxelNodeInterface() = default;

	virtual const FVoxelGraphNodeRef& GetNodeRef() const = 0;

public:
	template<typename T>
	using TValue = TVoxelFutureValue<T>;

	template<typename T>
	using TVoxelFutureValue [[deprecated]] = TValue<T>;

public:
	static void RaiseQueryErrorStatic(const FVoxelGraphNodeRef& Node, const UScriptStruct* QueryType);
	static void RaiseBufferErrorStatic(const FVoxelGraphNodeRef& Node);

	template<typename T>
	static void RaiseQueryErrorStatic(const FVoxelGraphNodeRef& Node)
	{
		IVoxelNodeInterface::RaiseQueryErrorStatic(Node, StaticStructFast<T>());
	}

public:
	void RaiseQueryError(const UScriptStruct* QueryType) const
	{
		RaiseQueryErrorStatic(GetNodeRef(), QueryType);
	}
	void RaiseBufferError() const
	{
		RaiseBufferErrorStatic(GetNodeRef());
	}
	template<typename T>
	void RaiseQueryError() const
	{
		IVoxelNodeInterface::RaiseQueryError(StaticStructFast<T>());
	}
};

template<typename T>
struct TVoxelMessageTokenFactory<T, std::enable_if_t<TIsDerivedFrom<T, IVoxelNodeInterface>::Value>>
{
	static TSharedRef<FVoxelMessageToken> CreateToken(const T& Node)
	{
		return Node.GetNodeRef().CreateMessageToken();
	}
};