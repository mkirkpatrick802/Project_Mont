// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphFactory.h"
#include "VoxelGraph.h"
#include "VoxelTerminalGraph.h"
#include "Interfaces/IMainFrameModule.h"
#include "Widgets/SVoxelNewGraphDialog.h"

UVoxelGraphFactory::UVoxelGraphFactory()
{
	SupportedClass = UVoxelGraph::StaticClass();

	bCreateNew = true;
	bEditAfterNew = true;
}

bool UVoxelGraphFactory::ConfigureProperties()
{
	const IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
	const TSharedPtr<SWindow> ParentWindow = MainFrame.GetParentWindow();

	const TSharedRef<SVoxelNewGraphDialog> NewGraphDialog =
		SNew(SVoxelNewGraphDialog, UVoxelGraph::StaticClass())
		.OnGetAssetCategory_Lambda([](const FAssetData& Item) -> FString
		{
			const UVoxelGraph* Graph = Cast<UVoxelGraph>(Item.GetAsset());
			if (!Graph)
			{
				return {};
			}
			return Graph->Category;
		})
		.OnGetAssetDescription_Lambda([](const FAssetData& Item) -> FString
		{
			const UVoxelGraph* Graph = Cast<UVoxelGraph>(Item.GetAsset());
			if (!Graph)
			{
				return {};
			}
			return Graph->Description;
		})
		.ShowAsset_Lambda([](const FAssetData& Item) -> bool
		{
			const UVoxelGraph* Graph = Cast<UVoxelGraph>(Item.GetAsset());
			if (!Graph)
			{
				return false;
			}
			return Graph->bUseAsTemplate;
		});

	FSlateApplication::Get().AddModalWindow(NewGraphDialog, ParentWindow);

	if (!NewGraphDialog->GetUserConfirmedSelection())
	{
		return false;
	}

	AssetToCopy = nullptr;

	TOptional<FAssetData> SelectedGraphAsset = NewGraphDialog->GetSelectedAsset();
	if (SelectedGraphAsset.IsSet())
	{
		AssetToCopy = Cast<UVoxelGraph>(SelectedGraphAsset->GetAsset());
		if (AssetToCopy)
		{
			return true;
		}

		const EAppReturnType::Type DialogResult = FMessageDialog::Open(
			EAppMsgType::OkCancel,
			EAppReturnType::Cancel,
			INVTEXT("The selected graph failed to load\nWould you like to create an empty Voxel Graph?"),
			INVTEXT("Create Default?"));

		if (DialogResult == EAppReturnType::Cancel)
		{
			return false;
		}
	}

	return true;
}

UObject* UVoxelGraphFactory::FactoryCreateNew(UClass* Class, UObject* InParent, const FName Name, const EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(Class->IsChildOf(UVoxelGraph::StaticClass()));

	if (!AssetToCopy)
	{
		return NewObject<UVoxelGraph>(InParent, Class, Name, Flags);
	}

	UVoxelGraph* NewGraph = DuplicateObject<UVoxelGraph>(AssetToCopy, InParent, Name);
	if (!ensure(NewGraph))
	{
		return nullptr;
	}

	NewGraph->Category = {};
	NewGraph->Description = {};
	return NewGraph;
}