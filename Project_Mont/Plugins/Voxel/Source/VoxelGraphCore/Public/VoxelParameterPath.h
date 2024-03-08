// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelParameterPath.generated.h"

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelParameterPath
{
	GENERATED_BODY()

public:
	TVoxelInlineArray<FGuid, 4> Guids;

	explicit FVoxelParameterPath() = default;

	//~ Begin TStructOpsTypeTraits Interface
	bool Serialize(FArchive& Ar);
	bool ExportTextItem(FString& ValueStr, const FVoxelParameterPath& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText, FArchive* InSerializingArchive = nullptr);
	//~ End TStructOpsTypeTraits Interface

public:
	static FVoxelParameterPath MakeEmpty();
	static FVoxelParameterPath Make(const FGuid& Guid);

	FString ToString() const;
	FVoxelParameterPath GetParent() const;
	FVoxelParameterPath MakeChild(const FGuid& Guid) const;
	FVoxelParameterPath MakeChild(const FVoxelParameterPath& Other) const;
	bool StartsWith(const FVoxelParameterPath& Other) const;
	FVoxelParameterPath MakeRelativeTo(const FVoxelParameterPath& BasePath) const;

public:
	FORCEINLINE bool IsEmpty() const
	{
		return Guids.IsEmpty();
	}
	FORCEINLINE int32 Num() const
	{
		return Guids.Num();
	}
	FORCEINLINE FGuid Leaf() const
	{
		return Guids.Last();
	}

public:
	FORCEINLINE bool operator==(const FVoxelParameterPath& Other) const
	{
		return Guids == Other.Guids;
	}
	FORCEINLINE bool operator!=(const FVoxelParameterPath& Other) const
	{
		return Guids != Other.Guids;
	}

	FORCEINLINE friend uint32 GetTypeHash(const FVoxelParameterPath& Path)
	{
		return FVoxelUtilities::MurmurHashBytes(
			MakeByteVoxelArrayView(Path.Guids),
			Path.Guids.Num());
	}
};

template<>
struct TStructOpsTypeTraits<FVoxelParameterPath> : TStructOpsTypeTraitsBase2<FVoxelParameterPath>
{
	enum
	{
		WithSerializer = true,
		WithIdenticalViaEquality = true,
		WithExportTextItem = true,
		WithImportTextItem = true,
	};
};