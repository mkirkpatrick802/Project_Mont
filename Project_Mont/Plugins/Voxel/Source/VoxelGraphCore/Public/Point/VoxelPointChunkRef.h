// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelGraphNodeRef.h"

struct FVoxelPointSet;
struct FVoxelChunkedPointSet;
class FVoxelRuntime;
class IVoxelRuntimeProvider;
class FVoxelDependencyTracker;

struct VOXELGRAPHCORE_API FVoxelPointChunkProviderRef
{
public:
	TWeakInterfacePtr<IVoxelRuntimeProvider> RuntimeProvider;
	TArray<FVoxelGraphNodeRef> TerminalGraphInstanceCallstack;
	FVoxelGraphNodeRef NodeRef;

	FString ToDebugString() const;
	TSharedPtr<FVoxelRuntime> GetRuntime(FString* OutError) const;
	TSharedPtr<const FVoxelChunkedPointSet> GetChunkedPointSet(FString* OutError = nullptr) const;

public:
	FORCEINLINE UWorld* GetWorld() const
	{
		checkVoxelSlow(IsInGameThread());
		if (const UObject* Object = RuntimeProvider.GetObject())
		{
			return Object->GetWorld();
		}
		return nullptr;
	}
	FORCEINLINE bool IsValid() const
	{
		return RuntimeProvider.IsValid();
	}
	FORCEINLINE bool operator==(const FVoxelPointChunkProviderRef& Other) const
	{
		return
			RuntimeProvider == Other.RuntimeProvider &&
			TerminalGraphInstanceCallstack == Other.TerminalGraphInstanceCallstack &&
			NodeRef == Other.NodeRef;
	}

	VOXELGRAPHCORE_API friend FArchive& operator<<(FArchive& Ar, FVoxelPointChunkProviderRef& Ref);
	VOXELGRAPHCORE_API friend uint32 GetTypeHash(const FVoxelPointChunkProviderRef& Ref);
};

struct VOXELGRAPHCORE_API FVoxelPointChunkRef
{
public:
	FVoxelPointChunkProviderRef ChunkProviderRef;
	FIntVector ChunkMin = FIntVector::ZeroValue;
	int32 ChunkSize = -1;

	TSharedPtr<const FVoxelPointSet> GetPoints(FString* OutError = nullptr) const;

	bool NetSerialize(FArchive& Ar, UPackageMap& Map);

public:
	FORCEINLINE UWorld* GetWorld() const
	{
		return ChunkProviderRef.GetWorld();
	}
	FORCEINLINE FVoxelBox GetBounds() const
	{
		return FVoxelBox(ChunkMin, ChunkMin + ChunkSize);
	}
	FORCEINLINE bool IsValid() const
	{
		return ChunkProviderRef.IsValid();
	}
	FORCEINLINE bool operator==(const FVoxelPointChunkRef& Other) const
	{
		return
			ChunkProviderRef == Other.ChunkProviderRef &&
			ChunkMin == Other.ChunkMin &&
			ChunkSize == Other.ChunkSize;
	}
	FORCEINLINE friend FArchive& operator<<(FArchive& Ar, FVoxelPointChunkRef& Ref)
	{
		Ar << Ref.ChunkProviderRef;
		Ar << Ref.ChunkMin;
		Ar << Ref.ChunkSize;
		return Ar;
	}
	FORCEINLINE friend uint32 GetTypeHash(const FVoxelPointChunkRef& Ref)
	{
		return FVoxelUtilities::MurmurHashMulti(
			GetTypeHash(Ref.ChunkProviderRef),
			GetTypeHash(Ref.ChunkMin),
			GetTypeHash(Ref.ChunkSize));
	}
};