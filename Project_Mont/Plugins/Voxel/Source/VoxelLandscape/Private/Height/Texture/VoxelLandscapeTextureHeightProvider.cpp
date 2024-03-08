// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Height/Texture/VoxelLandscapeTextureHeightProvider.h"
#include "Height/VoxelLandscapeHeightUtilities.h"
#include "TextureResource.h"
#include "Engine/Texture2D.h"

DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelLandscapeTextureHeightDataMemory);

FVoxelLandscapeTextureHeightCache* GVoxelLandscapeTextureCache = new FVoxelLandscapeTextureHeightCache();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelLandscapeTextureHeightProvider::TryInitialize()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!Texture)
	{
		return false;
	}

	TextureData = GVoxelLandscapeTextureCache->GetHeightData_GameThread(Texture, Channel);

	if (!TextureData)
	{
		return false;
	}

	return true;
}

FVoxelBox2D FVoxelLandscapeTextureHeightProvider::GetBounds() const
{
	const FVector2D Size
	{
		double(TextureData->SizeX),
		double(TextureData->SizeY)
	};
	return FVoxelBox2D(-Size / 2, Size / 2).Scale(ScaleXY);
}

FVoxelFuture FVoxelLandscapeTextureHeightProvider::Apply(const FVoxelLandscapeHeightQuery& Query) const
{
	if (TextureData->TypeSize == sizeof(uint8))
	{
		FVoxelLandscapeHeightUtilities::ApplyHeightmap(FVoxelLandscapeHeightUtilities::TArgs<uint8>
		{
			Query,
			ScaleXY,
			ScaleZ,
			OffsetZ,
			BicubicSmoothness,
			Interpolation,
			TextureData->SizeX,
			TextureData->SizeY,
			ReinterpretCastVoxelArrayView<uint8>(TextureData->Data)
		});
	}
	else if (TextureData->TypeSize == sizeof(uint16))
	{
		FVoxelLandscapeHeightUtilities::ApplyHeightmap(FVoxelLandscapeHeightUtilities::TArgs<uint16>
		{
			Query,
			ScaleXY,
			ScaleZ,
			OffsetZ,
			BicubicSmoothness,
			Interpolation,
			TextureData->SizeX,
			TextureData->SizeY,
			ReinterpretCastVoxelArrayView<uint16>(TextureData->Data)
		});
	}
	else if (TextureData->TypeSize == sizeof(float))
	{
		FVoxelLandscapeHeightUtilities::ApplyHeightmap(FVoxelLandscapeHeightUtilities::TArgs<float>
		{
			Query,
			ScaleXY,
			ScaleZ,
			OffsetZ,
			BicubicSmoothness,
			Interpolation,
			TextureData->SizeX,
			TextureData->SizeY,
			ReinterpretCastVoxelArrayView<float>(TextureData->Data)
		});
	}
	else
	{
		ensure(false);
	}

	return FVoxelFuture::Done();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelLandscapeTextureHeightCache::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	const double Time = FPlatformTime::Seconds();
	const double MinTime = Time - 30.;

	for (auto It = HeightDataRefs.CreateIterator(); It; ++It)
	{
		FHeightDataRef& HeightRef = *It;
		if (HeightRef.HeightData.GetSharedReferenceCount() > 1)
		{
			HeightRef.LastReferencedTime = Time;
			continue;
		}

		if (HeightRef.LastReferencedTime > MinTime)
		{
			continue;
		}

		It.RemoveCurrentSwap();
	}

	for (auto TextureIt = TextureToChannelToData.CreateIterator(); TextureIt; ++TextureIt)
	{
		for (auto ChannelIt = TextureIt.Value().CreateIterator(); ChannelIt; ++ChannelIt)
		{
			if (!ChannelIt->Value.IsValid())
			{
				ChannelIt.RemoveCurrent();
			}
		}

		if (TextureIt.Value().Num() == 0)
		{
			TextureIt.RemoveCurrent();
		}
	}
}

TSharedPtr<const FVoxelLandscapeTextureHeightData> FVoxelLandscapeTextureHeightCache::GetHeightData_GameThread(
	const UTexture2D* Texture,
	const EVoxelLandscapeTextureChannel Channel)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!ensure(Texture))
	{
		return nullptr;
	}

	FChannelToData& ChannelToData = TextureToChannelToData.FindOrAdd(Texture);

	if (const TSharedPtr<const FVoxelLandscapeTextureHeightData> HeightData = ChannelToData.FindRef(Channel).Pin())
	{
		return HeightData;
	}

	const TSharedPtr<FVoxelLandscapeTextureHeightData> HeightData = CreateHeightData_GameThread(Texture, Channel);
	if (!HeightData)
	{
		return nullptr;
	}

	HeightData->UpdateStats();

	ChannelToData.FindOrAdd(Channel) = HeightData;
	HeightDataRefs.Add(FHeightDataRef{ HeightData });

	return HeightData;
}

TSharedPtr<FVoxelLandscapeTextureHeightData> FVoxelLandscapeTextureHeightCache::CreateHeightData_GameThread(
	const UTexture2D* Texture,
	const EVoxelLandscapeTextureChannel Channel)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	const FTexturePlatformData* PlatformData = Texture->GetPlatformData();
	if (!ensure(PlatformData))
	{
		return nullptr;
	}

	const TSharedRef<FVoxelLandscapeTextureHeightData> HeightData = MakeVoxelShared<FVoxelLandscapeTextureHeightData>();

	const FTexture2DMipMap& Mip = PlatformData->Mips[0];
	HeightData->SizeX = Mip.SizeX;
	HeightData->SizeY = Mip.SizeY;

	const int64 Num = Mip.SizeX * Mip.SizeY;
	if (!ensure(Num < MAX_int32))
	{
		return nullptr;
	}

	if (PlatformData->PixelFormat == PF_R32_FLOAT)
	{
		const TSharedPtr<const TConstVoxelArrayView64<float>> Data = FVoxelUtilities::LockBulkData_ReadOnly<float>(Mip.BulkData);
		if (!ensure(Data) ||
			!ensure(Data->Num() == Num))
		{
			return nullptr;
		}

		HeightData->TypeSize = sizeof(float);
		FVoxelUtilities::SetNumFast(HeightData->Data, Num * sizeof(float));
		const TVoxelArrayView<float> Heights = ReinterpretCastVoxelArrayView<float>(HeightData->Data);

		FVoxelUtilities::Memcpy(Heights, *Data);

		HeightData->Range = FVoxelUtilities::GetMinMax(Heights);
		return HeightData;
	}

	if (PlatformData->PixelFormat == PF_R16_UINT)
	{
		const TSharedPtr<const TConstVoxelArrayView64<uint16>> Data = FVoxelUtilities::LockBulkData_ReadOnly<uint16>(Mip.BulkData);
		if (!ensure(Data) ||
			!ensure(Data->Num() == Num))
		{
			return nullptr;
		}

		HeightData->TypeSize = sizeof(uint16);
		FVoxelUtilities::SetNumFast(HeightData->Data, Num * sizeof(uint16));
		const TVoxelArrayView<uint16> Heights = ReinterpretCastVoxelArrayView<uint16>(HeightData->Data);

		FVoxelUtilities::Memcpy(Heights, *Data);

		const FInt32Interval Range = FVoxelUtilities::GetMinMax(Heights);
		HeightData->Range = FFloatInterval(Range.Min, Range.Max);
		return HeightData;
	}

	if (PlatformData->PixelFormat == PF_R8)
	{
		const TSharedPtr<const TConstVoxelArrayView64<uint8>> Data = FVoxelUtilities::LockBulkData_ReadOnly<uint8>(Mip.BulkData);
		if (!ensure(Data) ||
			!ensure(Data->Num() == Num))
		{
			return nullptr;
		}

		HeightData->TypeSize = sizeof(uint8);
		FVoxelUtilities::SetNumFast(HeightData->Data, Num * sizeof(uint8));
		const TVoxelArrayView<uint8> Heights = ReinterpretCastVoxelArrayView<uint8>(HeightData->Data);

		FVoxelUtilities::Memcpy(Heights, *Data);

		const FInt32Interval Range = FVoxelUtilities::GetMinMax(Heights);
		HeightData->Range = FFloatInterval(Range.Min, Range.Max);
		return HeightData;
	}

	if (PlatformData->PixelFormat == PF_B8G8R8A8)
	{
		const TSharedPtr<const TConstVoxelArrayView64<FColor>> Data = FVoxelUtilities::LockBulkData_ReadOnly<FColor>(Mip.BulkData);
		if (!ensure(Data) ||
			!ensure(Data->Num() == Num))
		{
			return nullptr;
		}
		const TConstVoxelArrayView64<FColor> Colors = *Data;

		HeightData->TypeSize = sizeof(uint8);
		FVoxelUtilities::SetNumFast(HeightData->Data, Num * sizeof(uint8));
		const TVoxelArrayView<uint8> Heights = ReinterpretCastVoxelArrayView<uint8>(HeightData->Data);

		switch (Channel)
		{
		default: ensure(false);
		case EVoxelLandscapeTextureChannel::R:
		{
			for (int32 Index = 0; Index < Num; Index++)
			{
				Heights[Index] = Colors[Index].R;
			}
		}
		break;
		case EVoxelLandscapeTextureChannel::G:
		{
			for (int32 Index = 0; Index < Num; Index++)
			{
				Heights[Index] = Colors[Index].G;
			}
		}
		break;
		case EVoxelLandscapeTextureChannel::B:
		{
			for (int32 Index = 0; Index < Num; Index++)
			{
				Heights[Index] = Colors[Index].B;
			}
		}
		break;
		case EVoxelLandscapeTextureChannel::A:
		{
			for (int32 Index = 0; Index < Num; Index++)
			{
				Heights[Index] = Colors[Index].A;
			}
		}
		break;
		}

		const FInt32Interval Range = FVoxelUtilities::GetMinMax(Heights);
		HeightData->Range = FFloatInterval(Range.Min, Range.Max);
		return HeightData;
	}

	ensure(false);
	return nullptr;
}