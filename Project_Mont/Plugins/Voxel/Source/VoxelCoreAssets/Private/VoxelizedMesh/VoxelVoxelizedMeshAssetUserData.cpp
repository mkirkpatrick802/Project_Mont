// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelizedMesh/VoxelVoxelizedMeshAssetUserData.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshAsset.h"

#include "Engine/StaticMesh.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"

UVoxelVoxelizedMeshAsset* UVoxelVoxelizedMeshAssetUserData::GetAsset(UStaticMesh& Mesh)
{
	if (const UVoxelVoxelizedMeshAssetUserData* AssetUserData = Mesh.GetAssetUserData<UVoxelVoxelizedMeshAssetUserData>())
	{
		if (UVoxelVoxelizedMeshAsset* Asset = AssetUserData->Asset.LoadSynchronous())
		{
			if (Asset->Mesh == &Mesh)
			{
				return Asset;
			}
		}
	}

	UVoxelVoxelizedMeshAsset* Asset = nullptr;

	// Try to find an existing one
	{
		TArray<FAssetData> AssetDatas;
		FARFilter Filter;
		Filter.ClassPaths.Add(UVoxelVoxelizedMeshAsset::StaticClass()->GetClassPathName());
		Filter.TagsAndValues.Add(GET_MEMBER_NAME_CHECKED(UVoxelVoxelizedMeshAsset, Mesh), Mesh.GetPathName());

		const IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		AssetRegistry.GetAssets(Filter, AssetDatas);

		if (AssetDatas.Num() > 1)
		{
			TArray<UObject*> Assets;
			for (const FAssetData& AssetData : AssetDatas)
			{
				Assets.Add(AssetData.GetAsset());
			}
			VOXEL_MESSAGE(Warning, "More than 1 voxelized mesh asset for mesh {0} found: {1}", Mesh, Assets);
		}

		for (const FAssetData& AssetData : AssetDatas)
		{
			UObject* Object = AssetData.GetAsset();
			if (!ensure(Object) ||
				!ensure(Object->IsA<UVoxelVoxelizedMeshAsset>()))
			{
				continue;
			}

			Asset = CastChecked<UVoxelVoxelizedMeshAsset>(Object);
			break;
		}

		if (!Asset &&
			AssetRegistry.IsLoadingAssets())
		{
			// Otherwise new assets are created for the same mesh
			VOXEL_MESSAGE(Error, "Asset registry is still loading assets - please refresh the voxel actor (Ctrl F5) once assets are loaded");
			return nullptr;
		}
	}

	if (!Asset)
	{
#if WITH_EDITOR
		// Create a new voxelized asset

		FString PackageName = FPackageName::ObjectPathToPackageName(Mesh.GetPathName());
		if (!PackageName.StartsWith("/Game/"))
		{
			// Don't create assets in the engine
			PackageName = "/Game/VoxelizedMeshes/" + Mesh.GetName();
		}

		Asset = FVoxelUtilities::CreateNewAsset_Direct<UVoxelVoxelizedMeshAsset>(PackageName, "_Voxelized");

		if (!Asset)
		{
			return nullptr;
		}

		Asset->Mesh = &Mesh;
#else
		// TODO Support this
		VOXEL_MESSAGE(Error, "Trying to create a voxelized mesh from {0} at runtime. Please do it in editor instead", Mesh);
		return nullptr;
#endif
	}

	ensure(Asset->Mesh == &Mesh);

	UVoxelVoxelizedMeshAssetUserData* AssetUserData = NewObject<UVoxelVoxelizedMeshAssetUserData>(&Mesh);
	AssetUserData->Asset = Asset;

	Mesh.AddAssetUserData(AssetUserData);
	Mesh.MarkPackageDirty();

	return Asset;
}