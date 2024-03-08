// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Height/Stamp/VoxelLandscapeHeightmapStampAsset.h"
#include "Height/Stamp/VoxelLandscapeHeightmapStampHeightProvider.h"
#include "Height/VoxelLandscapeHeightBrush.h"
#include "VoxelJumpFlood.h"

DEFINE_VOXEL_FACTORY(UVoxelLandscapeHeightmapStampAsset);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelLandscapeHeightmapStampAsset::Import(const TSharedRef<FVoxelLandscapeHeightmapStampData>& NewSourceStampData)
{
	VOXEL_FUNCTION_COUNTER();

	SourceStampData = NewSourceStampData;
	UpdateFromSource();

	(void)MarkPackageDirty();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelLandscapeHeightmapStampAsset::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER();

	Super::Serialize(Ar);

	if (Ar.IsLoading())
	{
		StampData = MakeVoxelShared<FVoxelLandscapeHeightmapStampData>();
		SourceStampData = MakeVoxelShared<FVoxelLandscapeHeightmapStampData>();
	}

	FVoxelUtilities::SerializeBulkData(this, BulkData, Ar, *StampData);

	bool HasEditorOnlyData = !Ar.IsCooking();
	Ar << HasEditorOnlyData;

	if (HasEditorOnlyData)
	{
		ensure(WITH_EDITORONLY_DATA);
		FVoxelUtilities::SerializeBulkData(this, SourceBulkData, Ar, *SourceStampData);
	}
}

#if WITH_EDITOR
void UVoxelLandscapeHeightmapStampAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostEditChangeProperty(PropertyChangedEvent);

	ON_SCOPE_EXIT
	{
		ForEachObjectOfClass<AVoxelLandscapeHeightBrush>([&](AVoxelLandscapeHeightBrush& Brush)
		{
			if (!Brush.Height.IsA<FVoxelLandscapeHeightmapStampHeightProvider>())
			{
				return;
			}

			if (Brush.Height.Get<FVoxelLandscapeHeightmapStampHeightProvider>().Stamp != this)
			{
				return;
			}

			Brush.QueueRecreate();
		});
	};

	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	UpdateFromSource();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelLandscapeHeightmapStampAsset::UpdateFromSource()
{
	VOXEL_FUNCTION_COUNTER();

	InternalScaleZ = 1.f;
	InternalOffsetZ = 0.f;

	if (!bEnableMinHeight)
	{
		StampData = MakeSharedCopy(*SourceStampData);
		return;
	}

	const int32 SizeX = SourceStampData->GetSizeX();
	const int32 SizeY = SourceStampData->GetSizeY();

	TVoxelArray<FIntPoint> Positions;
	FVoxelUtilities::SetNumFast(Positions, SizeX * SizeY);

	for (int32 Y = 0; Y < SizeY; Y++)
	{
		for (int32 X = 0; X < SizeX; X++)
		{
			const int32 Index = FVoxelUtilities::Get2DIndex<int32>(SizeX, SizeY, X, Y);
			const float Height = SourceStampData->GetNormalizedHeight(Index);

			if (Height <= MinHeight)
			{
				Positions[Index] = FIntPoint(MAX_int32);
			}
			else
			{
				Positions[Index] = FIntPoint(X, Y);
			}
		}
	}

	FVoxelJumpFlood::JumpFlood2D(FIntPoint(SizeX, SizeY), Positions);

	TVoxelArray64<float> Heights;
	FVoxelUtilities::SetNumFast(Heights, SizeX * SizeY);

	for (int32 Y = 0; Y < SizeY; Y++)
	{
		for (int32 X = 0; X < SizeX; X++)
		{
			const int32 Index = FVoxelUtilities::Get2DIndex<int32>(SizeX, SizeY, X, Y);
			const FIntPoint Position = Positions[Index];

			if (Position == FIntPoint(X, Y) ||
				Position == MAX_int32)
			{
				Heights[Index] = SourceStampData->GetNormalizedHeight(Index) - MinHeight;
				continue;
			}

			const float Distance = FVoxelUtilities::Size(Position - FIntPoint(X, Y));

			float Height = ScaleZ * (SourceStampData->GetNormalizedHeight(Position.X, Position.Y) - MinHeight);
			Height -= MinHeightSlope * ScaleXY * Distance;
			Heights[Index] = Height / ScaleZ;
		}
	}

	TVoxelArray64<uint16> NormalizedHeights;
	FVoxelUtilities::SetNumFast(NormalizedHeights, SizeX * SizeY);
	{
		const FFloatInterval MinMax = FVoxelUtilities::GetMinMax(Heights);
		const float Min = MinMax.Min;
		const float Max = MinMax.Max;

		for (int32 Index = 0; Index < SizeX * SizeY; Index++)
		{
			const float Value = (Heights[Index] - Min) / (Max - Min);
			NormalizedHeights[Index] = FVoxelUtilities::ClampToUINT16(FMath::RoundToInt(MAX_uint16 * Value));
		}

		InternalScaleZ = Max - Min;
		InternalOffsetZ = Min;
	}

	StampData = MakeVoxelShared<FVoxelLandscapeHeightmapStampData>();
	StampData->Initialize(SizeX, SizeY, MoveTemp(NormalizedHeights));

	(void)MarkPackageDirty();
}