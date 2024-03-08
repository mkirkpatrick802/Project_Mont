// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelExampleContentManager.h"
#include "VoxelPluginVersion.h"
#include "ImageUtils.h"
#include "HttpModule.h"
#include "Engine/Texture2D.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Slate/DeferredCleanupSlateBrush.h"

void FVoxelExampleContentManager::OnExamplesReady(const FSimpleDelegate& OnReady)
{
	if (ExampleContents.Num() > 0)
	{
		OnReady.ExecuteIfBound();
		return;
	}

	const FVoxelPluginVersion Version = FVoxelUtilities::GetPluginVersion();

	FString ContentVersion = "dev";
	if (Version.Type == FVoxelPluginVersion::EType::Release ||
		Version.Type == FVoxelPluginVersion::EType::Preview)
	{
		ContentVersion = FString::FromInt(Version.GetCounter());
	}

	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL("https://api.voxelplugin.dev/content/list?version=" + ContentVersion);
	Request->SetVerb("GET");
	Request->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr, const FHttpResponsePtr Response, const bool bConnectedSuccessfully)
	{
		if (ExampleContents.Num() > 0)
		{
			ensure(false);
			return;
		}

		if (!bConnectedSuccessfully ||
			!Response ||
			Response->GetResponseCode() != 200)
		{
			VOXEL_MESSAGE(Error, "Failed to query content: {0} {1}", Response->GetResponseCode(), Response->GetContentAsString());
			return;
		}

		const FString ContentString = Response->GetContentAsString();

		TSharedPtr<FJsonValue> ParsedValue;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ContentString);
		if (!ensure(FJsonSerializer::Deserialize(Reader, ParsedValue)) ||
			!ensure(ParsedValue))
		{
			return;
		}

		const TArray<TSharedPtr<FJsonValue>> Array = ParsedValue->AsArray();
		for (const TSharedPtr<FJsonValue>& Value : Array)
		{
			const TSharedPtr<FJsonObject> ContentObject = Value->AsObject();
			if (!ensure(ContentObject))
			{
				continue;
			}

			const TSharedRef<FVoxelExampleContent> Content = MakeVoxelShared<FVoxelExampleContent>();
			Content->Name = ContentObject->GetStringField(TEXT("name"));
			Content->Version = ContentVersion;
			Content->Size = ContentObject->GetNumberField(TEXT("size"));
			Content->Hash = ContentObject->GetStringField(TEXT("hash"));
			Content->DisplayName = ContentObject->GetStringField(TEXT("displayName"));
			Content->Description = ContentObject->GetStringField(TEXT("description"));
			Content->Thumbnail = MakeImage(ContentObject->GetStringField(TEXT("thumbnailHash")));
			Content->Image = MakeImage(ContentObject->GetStringField(TEXT("imageHash")));

			TArray<FString> CategoriesList;
			ContentObject->GetStringField(TEXT("categories")).ParseIntoArray(CategoriesList, TEXT(";"), false);

			if (ensure(CategoriesList.Num() > 0))
			{
				Content->Category = FName(CategoriesList[0]);

				for (int32 Index = 1; Index < CategoriesList.Num(); Index++)
				{
					if (CategoriesList[Index].IsEmpty())
					{
						continue;
					}

					Content->Tags.Add(FName(CategoriesList[Index]));
				}
			}

			ExampleContents.Add(Content);
		}

		OnReady.ExecuteIfBound();
	});
	Request->ProcessRequest();
}

TArray<TSharedPtr<FVoxelExampleContent>> FVoxelExampleContentManager::GetExamples() const
{
	ensure(ExampleContents.Num() > 0);
	return ExampleContents;
}

FLinearColor FVoxelExampleContentManager::GetContentTagColor(const FName ContentTag)
{
	const uint32 Hash = FCrc::StrCrc32(*ContentTag.ToString());
	const FLinearColor TagColor =  FLinearColor::MakeRandomSeededColor(Hash);
	return TagColor;
}

TSharedRef<TSharedPtr<ISlateBrushSource>> FVoxelExampleContentManager::MakeImage(const FString& Hash)
{
	static TMap<FString, TWeakPtr<TSharedPtr<ISlateBrushSource>>> Cache;
	if (const TSharedPtr<TSharedPtr<ISlateBrushSource>> CachedImage = Cache.FindRef(Hash).Pin())
	{
		return CachedImage.ToSharedRef();
	}

	const TSharedRef<TSharedPtr<ISlateBrushSource>> Result = MakeVoxelShared<TSharedPtr<ISlateBrushSource>>();
	Cache.Add(Hash, Result);

	if (!ensure(!Hash.IsEmpty()))
	{
		return Result;
	}

	const auto SetData = [Result](const TConstVoxelArrayView<uint8> Data)
	{
		FImage Image;
		if (!ensure(FImageUtils::DecompressImage(Data.GetData(), Data.Num(), Image)))
		{
			return;
		}

		UTexture2D* Texture = FImageUtils::CreateTexture2DFromImage(Image);
		if (!ensure(Texture))
		{
			return;
		}

		*Result = FDeferredCleanupSlateBrush::CreateBrush(Texture, FVector2D(Image.SizeX, Image.SizeY));
	};

	const FString Path = FVoxelUtilities::GetAppDataCache() / "ContentCache" / Hash + ".jpg";

	TArray<uint8> FileData;
	if (FFileHelper::LoadFileToArray(FileData, *Path))
	{
		if (ensure(FSHA1::HashBuffer(FileData.GetData(), FileData.Num()).ToString() == Hash))
		{
			IFileManager::Get().SetTimeStamp(*Path, FDateTime::UtcNow());
			SetData(FileData);
			return Result;
		}
	}

	const TSharedRef<IHttpRequest> UrlRequest = FHttpModule::Get().CreateRequest();
	UrlRequest->SetURL("https://api.voxelplugin.dev/content/images/download?hash=" + Hash);
	UrlRequest->SetVerb(TEXT("GET"));
	UrlRequest->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr, const FHttpResponsePtr UrlResponse, const bool bUrlConnectedSuccessfully)
	{
		if (!bUrlConnectedSuccessfully ||
			!UrlResponse ||
			UrlResponse->GetResponseCode() != 200)
		{
			VOXEL_MESSAGE(Error, "Failed to get image url {0}", Hash);
			return;
		}

		const TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
		Request->SetURL(UrlResponse->GetContentAsString());
		Request->SetVerb(TEXT("GET"));
		Request->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr, const FHttpResponsePtr Response, const bool bConnectedSuccessfully)
		{
			if (!bConnectedSuccessfully ||
				!Response ||
				Response->GetResponseCode() != 200)
			{
				VOXEL_MESSAGE(Error, "Failed to download image {0}", Hash);
				return;
			}

			const TArray<uint8> DownloadedData = Response->GetContent();

			if (!ensure(FSHA1::HashBuffer(DownloadedData.GetData(), DownloadedData.Num()).ToString() == Hash))
			{
				VOXEL_MESSAGE(Error, "Failed to download image {0}: invalid hash", Hash);
				return;
			}

			ensure(FFileHelper::SaveArrayToFile(DownloadedData, *Path));
			IFileManager::Get().SetTimeStamp(*Path, FDateTime::UtcNow());

			FVoxelUtilities::CleanupFileCache(FPaths::GetPath(Path), 10 * 1024 * 1024);

			SetData(DownloadedData);
		});
		Request->ProcessRequest();

	});
	UrlRequest->ProcessRequest();

	return Result;
}

FVoxelExampleContentManager& FVoxelExampleContentManager::Get()
{
	static FVoxelExampleContentManager Subsystem;
	return Subsystem;
}