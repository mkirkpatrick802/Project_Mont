// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "ContentBrowserModule.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshAsset.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshAssetUserData.h"

VOXEL_RUN_ON_STARTUP_EDITOR(RegisterVoxelizeMeshAction)
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();

	CBMenuExtenderDelegates.Add(MakeLambdaDelegate([](const TArray<FAssetData>& Assets)
	{
		TSharedRef<FExtender> Extender = MakeShared<FExtender>();

		for (const FAssetData& Asset : Assets)
		{
			if (!Asset.GetClass()->IsChildOf<UStaticMesh>())
			{
				return Extender;
			}
		}

		Extender->AddMenuExtension(
			"GetAssetActions",
			EExtensionHook::After,
			nullptr,
			MakeLambdaDelegate([Assets](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.AddMenuEntry(
					INVTEXT("Voxelize Mesh"),
					INVTEXT("Creates Voxelized Mesh Asset"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "ShowFlagsMenu.BSP"),
					FUIAction(FExecuteAction::CreateLambda([=]
					{
						TArray<UObject*> ObjectsToSync;
						for (const FAssetData& Asset : Assets)
						{
							UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset.GetAsset());
							if (!StaticMesh)
							{
								continue;
							}

							UVoxelVoxelizedMeshAsset* VoxelizedMesh = UVoxelVoxelizedMeshAssetUserData::GetAsset(*StaticMesh);
							if (!VoxelizedMesh)
							{
								continue;
							}

							ObjectsToSync.Add(VoxelizedMesh);
						}

						GEditor->SyncBrowserToObjects(ObjectsToSync);
					})));
			})
		);

		return Extender;
	}));
}