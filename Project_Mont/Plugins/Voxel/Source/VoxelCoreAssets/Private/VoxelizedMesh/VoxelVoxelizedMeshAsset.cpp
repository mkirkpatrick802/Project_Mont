// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelizedMesh/VoxelVoxelizedMeshAsset.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshData.h"

#include "MeshDescription.h"
#include "Engine/StaticMesh.h"

#if WITH_EDITOR
#include "AssetToolsModule.h"
#include "Editor/EditorEngine.h"
#include "EditorReimportHandler.h"
#include "DerivedDataCacheInterface.h"
#endif

DEFINE_VOXEL_FACTORY(UVoxelVoxelizedMeshAsset);

#if WITH_EDITOR
VOXEL_RUN_ON_STARTUP_GAME(RegisterVoxelVoxelizedMeshAssetOnReimport)
{
	FReimportManager::Instance()->OnPostReimport().AddLambda([](const UObject* Asset, bool bSuccess)
	{
		ForEachObjectOfClass_Copy<UVoxelVoxelizedMeshAsset>([&](UVoxelVoxelizedMeshAsset& VoxelizedMesh)
		{
			if (VoxelizedMesh.Mesh == Asset)
			{
				VoxelizedMesh.OnReimport();
			}
		});
	});
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelVoxelizedMeshAsset::OnReimport()
{
	VOXEL_FUNCTION_COUNTER();

	PrivateData.Reset();
	(void)GetData();
}

TSharedPtr<const FVoxelVoxelizedMeshData> UVoxelVoxelizedMeshAsset::GetData()
{
	if (!PrivateData)
	{
		PrivateData = CreateMeshData();
		OnDataChanged.Broadcast();
	}
	return PrivateData;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelVoxelizedMeshAsset::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	bool bCooked = Ar.IsCooking();
	Ar << bCooked;

	if (!bCooked ||
		IsTemplate() ||
		Ar.IsCountingMemory())
	{
		return;
	}

	if (Ar.IsLoading())
	{
		const TSharedRef<FVoxelVoxelizedMeshData> NewData = MakeVoxelShared<FVoxelVoxelizedMeshData>();
		NewData->Serialize(Ar);
		PrivateData = NewData;

		if (PrivateData->Size.IsZero())
		{
			PrivateData.Reset();
		}
	}
#if WITH_EDITOR
	else if (Ar.IsSaving())
	{
		if (!PrivateData)
		{
			PrivateData = CreateMeshData();
		}
		if (!PrivateData)
		{
			PrivateData = MakeVoxelShared<FVoxelVoxelizedMeshData>();
			ensure(PrivateData->Size.IsZero());
		}
		ConstCast(*PrivateData).Serialize(Ar);
	}
#endif
}

#if WITH_EDITOR
void UVoxelVoxelizedMeshAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	// Recompute data
	// Do this next frame to be able to undo on cancel
	FVoxelUtilities::DelayedCall([=]
	{
		FScopedSlowTask SlowTask(1.f, FText::FromString("Voxelizing " + GetPathName()));
		SlowTask.MakeDialog(true, true);
		SlowTask.EnterProgressFrame();

		const TSharedPtr<FVoxelVoxelizedMeshData> MeshData = CreateMeshData();

		if (SlowTask.ShouldCancel())
		{
			ensure(GEditor->UndoTransaction());
			return;
		}

		PrivateData = MeshData;
		OnDataChanged.Broadcast();
	});
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FVoxelVoxelizedMeshData> UVoxelVoxelizedMeshAsset::CreateMeshData() const
{
	VOXEL_FUNCTION_COUNTER();

	UStaticMesh* LoadedMesh = Mesh.LoadSynchronous();
	if (!LoadedMesh)
	{
		return nullptr;
	}

#if WITH_EDITOR
	FString KeySuffix;

	const FStaticMeshSourceModel& SourceModel = LoadedMesh->GetSourceModel(0);
	if (ensure(SourceModel.GetMeshDescriptionBulkData()))
	{
		KeySuffix += "MD";
		KeySuffix += SourceModel.GetMeshDescriptionBulkData()->GetIdString();
	}

	{
		FVoxelWriter Writer;
		Writer << ConstCast(VoxelSize);
		Writer << ConstCast(MaxSmoothness);
		Writer << ConstCast(VoxelizerSettings);

		KeySuffix += "_" + FVoxelUtilities::BlobToHex(Writer);
	}

	const FString DerivedDataKey = FDerivedDataCacheInterface::BuildCacheKey(
		TEXT("VOXEL_VOXELIZED_MESH"),
		TEXT("35EF96DA0A03463D8A5F7386E05BA4D5"),
		*KeySuffix);

	TArray<uint8> DerivedData;
	if (GetDerivedDataCacheRef().GetSynchronous(*DerivedDataKey, DerivedData, GetPathName()))
	{
		FMemoryReader Ar(DerivedData);

		const TSharedRef<FVoxelVoxelizedMeshData> MeshData = MakeVoxelShared<FVoxelVoxelizedMeshData>();
		MeshData->Serialize(Ar);
		return MeshData;
	}
	else
	{
		LOG_VOXEL(Log, "Voxelizing %s", *GetPathName());

		FScopedSlowTask SlowTask(1.f, FText::FromString("Voxelizing " + GetPathName()));
		// Only allow cancelling in PostEditChangeProperty
		SlowTask.MakeDialog(false, true);
		SlowTask.EnterProgressFrame();

		const TSharedPtr<FVoxelVoxelizedMeshData> MeshData = FVoxelVoxelizedMeshData::VoxelizeMesh(
			*LoadedMesh,
			VoxelSize,
			MaxSmoothness,
			VoxelizerSettings);

		if (!MeshData)
		{
			return nullptr;
		}

		FVoxelWriter Writer;
		ConstCast(*MeshData).Serialize(Writer.Ar());
		GetDerivedDataCacheRef().Put(*DerivedDataKey, Writer, GetPathName());

		return MeshData;
	}
#else
	return FVoxelVoxelizedMeshData::VoxelizeMesh(*LoadedMesh, VoxelSize, MaxSmoothness, VoxelizerSettings);
#endif
}