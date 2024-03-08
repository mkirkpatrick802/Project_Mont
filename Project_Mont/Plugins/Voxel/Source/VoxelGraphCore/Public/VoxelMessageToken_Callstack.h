// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMessage.h"
#include "Widgets/SCompoundWidget.h"
#include "VoxelMessageToken_Callstack.generated.h"

class FVoxelCallstack;

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelMessageToken_Callstack : public FVoxelMessageToken
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	TSharedPtr<const FVoxelCallstack> Callstack;

	//~ Begin FVoxelMessageToken Interface
	virtual uint32 GetHash() const override;
	virtual FString ToString() const override;
	virtual TSharedRef<IMessageToken> GetMessageToken() const override;
	//~ End FVoxelMessageToken Interface
};

#if WITH_EDITOR
class SVoxelCallstack : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(TSharedPtr<const FVoxelCallstack>, Callstack);
	};

	void Construct(const FArguments& Args);
};
#endif