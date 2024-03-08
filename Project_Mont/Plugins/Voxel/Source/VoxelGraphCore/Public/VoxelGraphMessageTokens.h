// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMessage.h"
#include "VoxelGraphNodeRef.h"
#include "VoxelGraphMessageTokens.generated.h"

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelMessageToken_NodeRef : public FVoxelMessageToken
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVoxelGraphNodeRef NodeRef;

	//~ Begin FVoxelMessageToken Interface
	virtual uint32 GetHash() const override;
	virtual FString ToString() const override;
	virtual TSharedRef<IMessageToken> GetMessageToken() const override;
	virtual void GetObjects(TSet<const UObject*>& OutObjects) const override;
	//~ End FVoxelMessageToken Interface
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelMessageToken_PinRef : public FVoxelMessageToken
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVoxelGraphPinRef PinRef;

	//~ Begin FVoxelMessageToken Interface
	virtual uint32 GetHash() const override;
	virtual FString ToString() const override;
	virtual TSharedRef<IMessageToken> GetMessageToken() const override;
	virtual void GetObjects(TSet<const UObject*>& OutObjects) const override;
	//~ End FVoxelMessageToken Interface
};