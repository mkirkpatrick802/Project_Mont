// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Point/VoxelPointStorageData.h"
#include "VoxelDependency.h"

FVoxelPointStorageData::FVoxelPointStorageData(const FName AssetName)
	: AssetName(AssetName)
	, Dependency(FVoxelDependency::Create(STATIC_FNAME("PointStorage"), AssetName))
{
}

void FVoxelPointStorageData::ClearData()
{
	VOXEL_FUNCTION_COUNTER();
	{
		VOXEL_SCOPE_LOCK(CriticalSection);
		ChunkRefToChunkData_RequiresLock.Empty();
	}
	Dependency->Invalidate();
}

void FVoxelPointStorageData::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER();

	using FVersion = DECLARE_VOXEL_VERSION
	(
		FirstVersion
	);

	int32 Version = FVersion::LatestVersion;
	Ar << Version;
	check(Version == FVersion::FirstVersion);

	if (Ar.IsSaving())
	{
		FVoxelWriter Writer;
		{
			VOXEL_SCOPE_LOCK(CriticalSection);

			// TODO
		}

		TVoxelArray64<uint8> CompressedData = FVoxelUtilities::Compress(Writer);
		CompressedData.BulkSerialize(Ar);
	}
	else
	{
		check(Ar.IsLoading());

		// Invalidate outside of the lock
		FVoxelDependencyInvalidationScope InvalidationScope;

		TArray64<uint8> CompressedData;
		CompressedData.BulkSerialize(Ar);

		TVoxelArray64<uint8> Data;
		if (!ensure(FVoxelUtilities::Decompress(CompressedData, Data)))
		{
			return;
		}

		if (Data.Num() == 0)
		{
			ClearData();
			return;
		}

		FVoxelReader Reader(Data);

		VOXEL_SCOPE_LOCK(CriticalSection);

		// TODO

		Dependency->Invalidate();
	}
}

TSharedRef<FVoxelPointStorageChunkData> FVoxelPointStorageData::FindOrAddChunkData(const FVoxelPointChunkRef& ChunkRef)
{
	VOXEL_SCOPE_LOCK(CriticalSection);

	TSharedPtr<FVoxelPointStorageChunkData>& ChunkData = ChunkRefToChunkData_RequiresLock.FindOrAdd(ChunkRef);
	if (!ChunkData)
	{
		ChunkData = MakeVoxelShared<FVoxelPointStorageChunkData>(FVoxelDependency::Create(
			STATIC_FNAME("PointStorage.SpawnableData"),
			FName(FString::Printf(TEXT("%s %s"),
				*ChunkRef.ChunkProviderRef.ToDebugString(),
				*ChunkRef.ChunkMin.ToString()))));
	}
	return ChunkData.ToSharedRef();
}

bool FVoxelPointStorageData::SetPointAttribute(
	const FVoxelPointHandle& Handle,
	const FName Name,
	const FVoxelPinType& Type,
	const FVoxelPinValue& Value,
	FString* OutError)
{
	VOXEL_FUNCTION_COUNTER();
	const FVoxelDependencyInvalidationScope InvalidationScope;

	if (!Value.IsValid())
	{
		if (OutError)
		{
			*OutError = "Invalid Value";
		}
		return false;
	}

	if (Value.GetType() != Type.GetInnerExposedType())
	{
		if (OutError)
		{
			*OutError = "Value has incorrect type";
		}
		return false;
	}

	if (!Type.IsBuffer())
	{
		if (OutError)
		{
			*OutError = "Type should be a buffer type";
		}
		return false;
	}

	if (!Handle.IsValid())
	{
		if (OutError)
		{
			*OutError = "Invalid Handle";
		}
		return false;
	}

	const TSharedRef<FVoxelPointStorageChunkData> ChunkData = FindOrAddChunkData(Handle.ChunkRef);

	VOXEL_SCOPE_LOCK(ChunkData->CriticalSection);

	TSharedPtr<FVoxelPointStorageChunkData::FAttribute>& AttributeOverride = ChunkData->NameToAttributeOverride.FindOrAdd(Name);
	if (!AttributeOverride)
	{
		Dependency->Invalidate();

		AttributeOverride = MakeVoxelShared<FVoxelPointStorageChunkData::FAttribute>();
		AttributeOverride->Buffer = MakeVoxelShared<FVoxelBufferBuilder>(Type.GetInnerType());
	}
	else if (AttributeOverride->Buffer->InnerType != Type.GetInnerType())
	{
		if (OutError)
		{
			*OutError =
				"Attribute " + Name.ToString() + " is already stored as " +
				AttributeOverride->Buffer->InnerType.GetExposedType().ToString() + ". " +
				"Cannot store it as " +
				Type.GetInnerExposedType().ToString();
		}
		return false;
	}

	bool bChanged = false;
	int32 Index;
	if (const int32* ExistingIndexPtr = AttributeOverride->PointIdToIndex.Find(Handle.PointId))
	{
		Index = *ExistingIndexPtr;
	}
	else
	{
		bChanged = true;
		Index = AttributeOverride->Buffer->AddZeroed(1);
		AttributeOverride->PointIdToIndex.Add_CheckNew(Handle.PointId, Index);
	}

	const FVoxelRuntimePinValue RuntimeValue = FVoxelPinType::MakeRuntimeValue(Type, Value);
	if (!ensure(RuntimeValue.IsValid()))
	{
		return false;
	}

	bChanged |= AttributeOverride->Buffer->CopyFrom(
		RuntimeValue.Get<FVoxelBuffer>(),
		0,
		Index);

	if (bChanged)
	{
		ChunkData->Dependency->Invalidate();
	}

	return true;
}