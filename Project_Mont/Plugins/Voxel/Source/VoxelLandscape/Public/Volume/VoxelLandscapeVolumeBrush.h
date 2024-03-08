// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelActorBase.h"
#include "VoxelLandscapeVolumeBrush.generated.h"

class AVoxelLandscapeVolumeBrush;
struct FVoxelLandscapeVolumeProvider;

UENUM(BlueprintType)
enum class EVoxelLandscapeVolumeBlendMode : uint8
{
	Additive,
	Subtractive
};

class VOXELLANDSCAPE_API FVoxelLandscapeVolumeBrushRuntime : public IVoxelActorRuntime
{
public:
	const FObjectKey World;
	const FTransform LocalToWorld;
	const EVoxelLandscapeVolumeBlendMode BlendMode;
	const float Smoothness;
	const int32 Priority;
	const TSharedRef<FVoxelLandscapeVolumeProvider> VolumeProvider;

	static TSharedPtr<FVoxelLandscapeVolumeBrushRuntime> Create(const AVoxelLandscapeVolumeBrush& Brush);

public:
	FVoxelBox GetBounds(const FMatrix& WorldToVoxel) const;

	//~ Begin IVoxelActorRuntime Interface
	virtual void Destroy() override;
	//~ End IVoxelActorRuntime Interface

private:
	FVoxelBox PrivateLocalBounds;
	int32 BrushIndex = -1;

	explicit FVoxelLandscapeVolumeBrushRuntime(const AVoxelLandscapeVolumeBrush& Brush);

	void RegisterBrush();
};

UCLASS()
class VOXELLANDSCAPE_API AVoxelLandscapeVolumeBrush : public AVoxelActorBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	EVoxelLandscapeVolumeBlendMode BlendMode = EVoxelLandscapeVolumeBlendMode::Additive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config, meta = (Units = cm))
	float Smoothness = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Config)
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, Category = "Config", meta = (BaseStruct = "/Script/VoxelLandscape.VoxelLandscapeVolumeProvider"))
#if CPP
	TVoxelInstancedStruct<FVoxelLandscapeVolumeProvider> Volume;
#else
	FVoxelInstancedStruct Volume;
#endif

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