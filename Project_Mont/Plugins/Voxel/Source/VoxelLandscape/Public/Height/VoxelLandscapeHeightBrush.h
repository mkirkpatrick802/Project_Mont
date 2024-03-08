// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelActorBase.h"
#include "Height/VoxelLandscapeHeightBlendMode.h"
#include "VoxelLandscapeHeightBrush.generated.h"

class FVoxelLandscapeState;
class AVoxelLandscapeHeightBrush;
struct FVoxelLandscapeChunkKey;
struct FVoxelLandscapeHeightQuery;
struct FVoxelLandscapeHeightProvider;

class VOXELLANDSCAPE_API FVoxelLandscapeHeightBrushRuntime : public IVoxelActorRuntime
{
public:
	const FObjectKey World;
	const FTransform2d LocalToWorld;
	const float ScaleZ;
	const float OffsetZ;
	const EVoxelLandscapeHeightBlendMode BlendMode;
	const float Smoothness;
	const int32 Priority;
	const TSharedRef<FVoxelLandscapeHeightProvider> HeightProvider;
	const float MinHeight;
	const float MaxHeight;

	static TSharedPtr<FVoxelLandscapeHeightBrushRuntime> Create(const AVoxelLandscapeHeightBrush& Brush);

public:
	FVoxelBox2D GetBounds(const FTransform2d& WorldToVoxel) const;

	// Start and Step are in voxel space
	FVoxelLandscapeHeightQuery MakeQuery(
		const FVoxelLandscapeState& State,
		const FVector2D& Start,
		double Step,
		const FIntPoint& Size,
		TVoxelArrayView<float> Heights) const;

	//~ Begin IVoxelActorRuntime Interface
	virtual void Destroy() override;
	//~ End IVoxelActorRuntime Interface

private:
	FVoxelBox2D PrivateLocalBounds;
	int32 BrushIndex = -1;

	explicit FVoxelLandscapeHeightBrushRuntime(const AVoxelLandscapeHeightBrush& Brush);

	void RegisterBrush();
};

UCLASS()
class VOXELLANDSCAPE_API AVoxelLandscapeHeightBrush : public AVoxelActorBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	EVoxelLandscapeHeightBlendMode BlendMode = EVoxelLandscapeHeightBlendMode::Max;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config, meta = (Units = cm))
	float Smoothness = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, Category = Config, meta = (BaseStruct = "/Script/VoxelLandscape.VoxelLandscapeHeightProvider"))
#if CPP
	TVoxelInstancedStruct<FVoxelLandscapeHeightProvider> Height;
#else
	FVoxelInstancedStruct Height;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config, AdvancedDisplay)
	float MinHeight = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config, AdvancedDisplay)
	float MaxHeight = 1e9;

public:
	//~ Begin AActor Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End AActor Interface

public:
	//~ Begin AVoxelActorBase Interface
	virtual void NotifyTransformChanged() override;
	virtual bool ShouldDestroyWhenHidden() const override;
	virtual FVoxelBox GetLocalBounds() const override;
	//~ End AVoxelActorBase Interface

protected:
	//~ Begin AVoxelActorBase Interface
	virtual TSharedPtr<IVoxelActorRuntime> CreateNewRuntime() override;
	//~ End AVoxelActorBase Interface
};