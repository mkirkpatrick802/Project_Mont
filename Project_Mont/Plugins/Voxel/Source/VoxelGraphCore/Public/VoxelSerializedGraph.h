// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPinValue.h"
#include "VoxelSerializedGraph.generated.h"

struct FVoxelNode;

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelSerializedPinRef
{
	GENERATED_BODY()

	UPROPERTY()
	FName NodeName;

	UPROPERTY()
	FName PinName;

	UPROPERTY()
	bool bIsInput = false;
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelSerializedPin
{
	GENERATED_BODY()

	UPROPERTY()
	FVoxelPinType Type;

	UPROPERTY()
	FName PinName;

	UPROPERTY()
	FName ParentPinName;

	UPROPERTY()
	FVoxelPinValue DefaultValue;

	UPROPERTY()
	TArray<FVoxelSerializedPinRef> LinkedTo;
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelSerializedNode
{
	GENERATED_BODY()

public:
	UPROPERTY()
#if CPP
	TVoxelInstancedStruct<FVoxelNode> VoxelNode;
#else
	FVoxelInstancedStruct VoxelNode;
#endif

	UPROPERTY()
	TMap<FName, FVoxelSerializedPin> InputPins;

	UPROPERTY()
	TMap<FName, FVoxelSerializedPin> OutputPins;

public:
	UPROPERTY()
	FName StructName;

	UPROPERTY()
	FName EdGraphNodeName;

	UPROPERTY()
	FName EdGraphNodeTitle;

	UPROPERTY()
	TArray<FString> Warnings;

	UPROPERTY()
	TArray<FString> Errors;

public:
	FName GetNodeId() const;
	FVoxelSerializedPin* FindPin(FName PinName, bool bIsInput);
	TSharedRef<FVoxelMessageToken> CreateMessageToken() const;
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelSerializedGraph
{
	GENERATED_BODY()

public:
	using FVersion = DECLARE_VOXEL_VERSION
	(
		FirstVersion,
		AddOutputGuids
	);

	UPROPERTY()
	bool bIsValid = false;

	UPROPERTY()
	int32 Version = 0;

	UPROPERTY()
	TSet<FGuid> OutputGuids;

	UPROPERTY()
	TMap<FName, FVoxelSerializedNode> NodeNameToNode;

	UPROPERTY()
	TMap<FVoxelPinType, FVoxelInstancedStruct> TypeToMakeNode;

	UPROPERTY()
	TMap<FVoxelPinType, FVoxelInstancedStruct> TypeToBreakNode;

public:
	UPROPERTY()
	TSet<FName> DebuggedNodes;

	UPROPERTY()
	FVoxelSerializedPinRef PreviewedPin;

	UPROPERTY()
	FVoxelPinType PreviewedPinType;

	UPROPERTY()
	FVoxelInstancedStruct PreviewHandler;

public:
	FVoxelSerializedPin* FindPin(const FVoxelSerializedPinRef& Ref);
	const FVoxelSerializedPin* FindPin(const FVoxelSerializedPinRef& Ref) const
	{
		return ConstCast(this)->FindPin(Ref);
	}

	const FVoxelNode* FindMakeNode(const FVoxelPinType& Type) const;
	const FVoxelNode* FindBreakNode(const FVoxelPinType& Type) const;
};