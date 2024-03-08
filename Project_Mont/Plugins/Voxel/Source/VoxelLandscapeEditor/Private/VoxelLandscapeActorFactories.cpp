// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelLandscapeActorFactories.h"
#include "VoxelLandscape.h"
#include "Height/VoxelLandscapeHeightBrush.h"
#include "Height/Stamp/VoxelLandscapeHeightmapStampAsset.h"
#include "Height/Stamp/VoxelLandscapeHeightmapStampHeightProvider.h"
#include "Volume/VoxelLandscapeVolumeBrush.h"

DEFINE_VOXEL_PLACEABLE_ITEM_FACTORY(UActorFactoryVoxelLandscape);
DEFINE_VOXEL_PLACEABLE_ITEM_FACTORY(UActorFactoryVoxelHeightBrush);
DEFINE_VOXEL_PLACEABLE_ITEM_FACTORY(UActorFactoryVoxelVolumeBrush);

UActorFactoryVoxelLandscape::UActorFactoryVoxelLandscape()
{
	DisplayName = INVTEXT("Voxel Landscape");
	NewActorClass = AVoxelLandscape::StaticClass();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UActorFactoryVoxelHeightBrush::UActorFactoryVoxelHeightBrush()
{
	DisplayName = INVTEXT("Voxel Landscape Height Brush");
	NewActorClass = AVoxelLandscapeHeightBrush::StaticClass();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UActorFactoryVoxelHeightBrush_FromStamp::UActorFactoryVoxelHeightBrush_FromStamp()
{
	DisplayName = INVTEXT("Voxel Landscape Height Brush");
	NewActorClass = AVoxelLandscapeHeightBrush::StaticClass();
}

bool UActorFactoryVoxelHeightBrush_FromStamp::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	return
		AssetData.IsValid() &&
		AssetData.GetClass()->IsChildOf<UVoxelLandscapeHeightmapStampAsset>();
}

void UActorFactoryVoxelHeightBrush_FromStamp::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	FVoxelLandscapeHeightmapStampHeightProvider Provider;
	Provider.Stamp = CastChecked<UVoxelLandscapeHeightmapStampAsset>(Asset);

	AVoxelLandscapeHeightBrush* TypedActor = CastChecked<AVoxelLandscapeHeightBrush>(NewActor);
	TypedActor->Height = Provider;
	TypedActor->QueueRecreate();
}

UObject* UActorFactoryVoxelHeightBrush_FromStamp::GetAssetFromActorInstance(AActor* ActorInstance)
{
	AVoxelLandscapeHeightBrush* TypedActor = Cast<AVoxelLandscapeHeightBrush>(ActorInstance);
	if (!ensure(TypedActor))
	{
		return nullptr;
	}

	const FVoxelLandscapeHeightmapStampHeightProvider* Provider = TypedActor->Height.GetPtr<FVoxelLandscapeHeightmapStampHeightProvider>();
	if (!Provider)
	{
		return nullptr;
	}

	return Provider->Stamp;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UActorFactoryVoxelVolumeBrush::UActorFactoryVoxelVolumeBrush()
{
	DisplayName = INVTEXT("Voxel Landscape Volume Brush");
	NewActorClass = AVoxelLandscapeVolumeBrush::StaticClass();
}