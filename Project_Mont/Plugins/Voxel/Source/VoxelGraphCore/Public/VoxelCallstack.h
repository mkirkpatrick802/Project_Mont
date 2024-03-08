// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelGraphNodeRef.h"

class IVoxelRuntimeProvider;

class VOXELGRAPHCORE_API FVoxelCallstack : public TSharedFromThis<FVoxelCallstack>
{
public:
	FVoxelCallstack() = default;

	uint32 GetHash() const;
	FString ToDebugString() const;
	TArray<FVoxelGraphNodeRef> Array() const;
	TSharedRef<FVoxelMessageToken> CreateMessageToken() const;

	TSharedRef<const FVoxelCallstack> MakeChild(
		FName ChildPinName,
		const FVoxelGraphNodeRef& ChildNodeRef,
		const TWeakObjectPtr<UObject>& ChildRuntimeProvider) const;

	FORCEINLINE bool IsValid() const
	{
		return !NodeRef.IsExplicitlyNull() || Parent;
	}
	FORCEINLINE FName GetPinName() const
	{
		return PinName;
	}
	FORCEINLINE const FVoxelGraphNodeRef& GetNodeRef() const
	{
		return NodeRef;
	}
	FORCEINLINE const TWeakObjectPtr<UObject>& GetRuntimeProvider() const
	{
		return RuntimeProvider;
	}
	FORCEINLINE const FVoxelCallstack* GetParent() const
	{
		return Parent.Get();
	}

private:
	FName PinName;
	FVoxelGraphNodeRef NodeRef;
	TWeakObjectPtr<UObject> RuntimeProvider;
	TSharedPtr<const FVoxelCallstack> Parent;
};