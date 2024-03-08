// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Channel/VoxelChannelManager.h"
#include "Channel/VoxelWorldChannel.h"
#include "Channel/VoxelChannelRegistry.h"
#include "VoxelSettings.h"
#include "Engine/Engine.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "AssetRegistry/AssetData.h"

FVoxelChannelManager* GVoxelChannelManager = new FVoxelChannelManager();

VOXEL_CONSOLE_COMMAND(
	LogAllBrushes,
	"voxel.LogAllBrushes",
	"")
{
	GVoxelChannelManager->LogAllBrushes_GameThread();
}

VOXEL_CONSOLE_COMMAND(
	LogAllChannels,
	"voxel.LogAllChannels",
	"")
{
	GVoxelChannelManager->LogAllChannels_GameThread();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelChannelManager::IsReady(const bool bLog) const
{
	if (!bChannelRegistriesLoaded)
	{
		if (bLog)
		{
			LOG_VOXEL(Log, "Not ready: bChannelRegistriesLoaded = false");
		}
		return false;
	}

	if (PendingHandles.Num() == 0)
	{
		return true;
	}

	if (bLog)
	{
		for (const auto& Handle : PendingHandles)
		{
			LOG_VOXEL(Log, "Not ready: waiting for %s to load", *Handle->GetDebugName());
		}
	}
	return false;
}

TArray<FName> FVoxelChannelManager::GetValidChannelNames() const
{
	VOXEL_SCOPE_LOCK(CriticalSection);

	TArray<FName> Result;
	for (const auto& AssetIt : AssetToChannelDefinitions_RequiresLock)
	{
		for (const auto& It : AssetIt.Value)
		{
			Result.Add(It.Key);
		}
	}
	return Result;
}

TArray<const UObject*> FVoxelChannelManager::GetChannelAssets() const
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	TArray<const UObject*> Assets;
	for (const auto& It : AssetToChannelDefinitions_RequiresLock)
	{
		if (ensureVoxelSlow(It.Key))
		{
			Assets.Add(It.Key);
		}
	}
	return Assets;
}

TOptional<FVoxelChannelDefinition> FVoxelChannelManager::FindChannelDefinition(const FName Name) const
{
	VOXEL_SCOPE_LOCK(CriticalSection);

	const FVoxelChannelDefinition* Definition = nullptr;
	for (const auto& It : AssetToChannelDefinitions_RequiresLock)
	{
		if (const FVoxelChannelDefinition* NewDefinition = It.Value.Find(Name))
		{
			ensure(!Definition);
			Definition = NewDefinition;
		}
	}
	if (!Definition)
	{
		if (!IsReady(true))
		{
			VOXEL_MESSAGE(Error, "Querying channels before channel registry assets are done loading");
		}
		return {};
	}
	return *Definition;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelChannelManager::LogAllBrushes_GameThread()
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	for (const UWorld* World : TObjectRange<UWorld>())
	{
		const TSharedRef<FVoxelWorldChannelManager> ChannelManager = FVoxelWorldChannelManager::Get(World);

		VOXEL_SCOPE_LOCK(ChannelManager->CriticalSection);

		for (const auto& ChannelIt : ChannelManager->Channels_RequiresLock)
		{
			const FVoxelWorldChannel& Channel = *ChannelIt.Value;

			VOXEL_SCOPE_LOCK(Channel.CriticalSection);
			for (const TSharedPtr<const FVoxelBrush>& Brush : Channel.Brushes_RequiresLock)
			{
				LOG_VOXEL(Log, "World: %s: Channel %s: Brush %s: Priority=%llu LocalBounds=%s",
					*World->GetPathName(),
					*Channel.Definition.Name.ToString(),
					*Brush->DebugName.ToString(),
					Brush->Priority.Raw,
					Brush->LocalBounds.IsInfinite() ? TEXT("Infinite") : *Brush->LocalBounds.ToString());
			}
		}
	}
}

void FVoxelChannelManager::LogAllChannels_GameThread()
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	LOG_VOXEL(Log, "Global channels:");
	for (const auto& AssetIt : AssetToChannelDefinitions_RequiresLock)
	{
		LOG_VOXEL(Log, "\t%s:", AssetIt.Key ? *AssetIt.Key->GetPathName() : TEXT(""));

		for (const auto& It : AssetIt.Value)
		{
			LOG_VOXEL(Log, "\t\t%s", *It.Value.ToString());
		}
	}

	LOG_VOXEL(Log, "World channels:");
	for (const UWorld* World : TObjectRange<UWorld>())
	{
		const TSharedRef<FVoxelWorldChannelManager> ChannelManager = FVoxelWorldChannelManager::Get(World);

		VOXEL_SCOPE_LOCK(ChannelManager->CriticalSection);
		LOG_VOXEL(Log, "\t%s:", *World->GetPathName());

		for (const auto& ChannelIt : ChannelManager->Channels_RequiresLock)
		{
			LOG_VOXEL(Log, "\t\t%s (%d brushes)",
				*ChannelIt.Value->Definition.ToString(),
				ChannelIt.Value->Brushes_RequiresLock.Num());
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelChannelManager::RegisterChannel(const FVoxelChannelDefinition& ChannelDefinition)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	for (const auto& It : AssetToChannelDefinitions_RequiresLock)
	{
		if (It.Value.Contains(ChannelDefinition.Name))
		{
			return false;
		}
	}

	static UVoxelChannelRegistry* ManualChannelRegistry = nullptr;
	if (!ManualChannelRegistry)
	{
		ManualChannelRegistry = NewObject<UVoxelChannelRegistry>(GetTransientPackage(), "ManualChannelRegistry");
		ManualChannelRegistry->AddToRoot();
	}

	AssetToChannelDefinitions_RequiresLock.FindOrAdd(ManualChannelRegistry).Add_EnsureNew(ChannelDefinition.Name, ChannelDefinition);
	bRefreshQueued = true;
	return true;
}

void FVoxelChannelManager::UpdateChannelsFromAsset_GameThread(
	const UObject* Asset,
	const FString& Prefix,
	const TArray<FVoxelChannelExposedDefinition>& Channels)
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelMap<FName, FVoxelChannelDefinition> NewChannelDefinitions;
	for (const FVoxelChannelExposedDefinition& Channel : Channels)
	{
		const FName Name = Prefix + "." + Channel.Name;

		NewChannelDefinitions.Add_EnsureNew(Name, FVoxelChannelDefinition
		{
			Name,
			Channel.Type,
			FVoxelPinType::MakeRuntimeValue(Channel.Type, Channel.DefaultValue)
		});
	}

	{
		VOXEL_SCOPE_LOCK(CriticalSection);

		TVoxelMap<FName, FVoxelChannelDefinition>& ChannelDefinitions = AssetToChannelDefinitions_RequiresLock.FindOrAdd(Asset);
		if (ChannelDefinitions.OrderIndependentEqual(NewChannelDefinitions))
		{
			return;
		}

		ChannelDefinitions = NewChannelDefinitions;
	}

	bRefreshQueued = true;
}

void FVoxelChannelManager::RemoveChannelsFromAsset_GameThread(const UObject* Asset)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);
	ensure(AssetToChannelDefinitions_RequiresLock.Remove(Asset));
	bRefreshQueued = true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelChannelManager::Initialize()
{
	VOXEL_FUNCTION_COUNTER();

	GetMutableDefault<UVoxelSettings>()->UpdateChannels();

	// Nothing should depend on channels so early, don't trigger a refresh
	ClearQueuedRefresh();
	// Force a tick now in case assets are loaded
	Tick();
	// Nothing should depend on channels so early, don't trigger a refresh
	ClearQueuedRefresh();
}

void FVoxelChannelManager::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	if (bRefreshQueued)
	{
		bRefreshQueued = false;

		OnChannelDefinitionsChanged_GameThread.Broadcast();

		for (const TSharedRef<FVoxelWorldChannelManager>& Subsystem : FVoxelWorldChannelManager::GetAll())
		{
			VOXEL_SCOPE_LOCK(Subsystem->CriticalSection);
			Subsystem->Channels_RequiresLock.Reset();
		}

		if (ensure(GEngine))
		{
			GEngine->Exec(nullptr, TEXT("voxel.RefreshAll"));
		}
	}

	if (!bChannelRegistriesLoaded &&
		// No engine in commandlets
		GEngine)
	{
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

		if (!FPlatformProperties::RequiresCookedData() &&
			!AssetRegistry.IsSearchAllAssets())
		{
			// Force search all assets in standalone
			AssetRegistry.SearchAllAssets(false);
		}

		if (!AssetRegistry.IsLoadingAssets())
		{
			bChannelRegistriesLoaded = true;

			TArray<FAssetData> AssetDatas;
			FARFilter Filter;
			Filter.ClassPaths.Add(UVoxelChannelRegistry::StaticClass()->GetClassPathName());
			AssetRegistry.GetAssets(Filter, AssetDatas);

			AssetRegistry.OnAssetAdded().AddLambda([](const FAssetData& AssetData)
			{
				ensure(GIsEditor);

				if (AssetData.GetClass() != StaticClassFast<UVoxelChannelRegistry>())
				{
					return;
				}

				ensure(TSoftObjectPtr<UVoxelChannelRegistry>(AssetData.GetSoftObjectPath()).LoadSynchronous());
			});

			FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
			for (const FAssetData& AssetData : AssetDatas)
			{
				const TSharedPtr<FStreamableHandle> Handle =
					GetDefault<UVoxelSettings>()->bAsyncLoadChannelRegistries
					? StreamableManager.RequestAsyncLoad(AssetData.GetSoftObjectPath())
					: StreamableManager.RequestSyncLoad(AssetData.GetSoftObjectPath());

				if (!ensure(Handle))
				{
					continue;
				}

				ConstCast(Handle->GetDebugName()) = AssetData.GetSoftObjectPath().ToString();
				PendingHandles.Add(Handle);
			}
		}
	}

	PendingHandles.RemoveAllSwap([](const TSharedPtr<FStreamableHandle>& Handle)
	{
		return Handle->HasLoadCompleted();
	});

	for (const TSharedPtr<FStreamableHandle>& Handle : PendingHandles)
	{
		LOG_VOXEL(Verbose, "Waiting for %s", *Handle->GetDebugName());
	}
}

void FVoxelChannelManager::AddReferencedObjects(FReferenceCollector& Collector)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	for (auto It = AssetToChannelDefinitions_RequiresLock.CreateIterator(); It; ++It)
	{
		TObjectPtr<const UObject> Object = It.Key();
		Collector.AddReferencedObject(Object);

		if (!ensureVoxelSlow(Object))
		{
			It.RemoveCurrent();
		}
	}
}