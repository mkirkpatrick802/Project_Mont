// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelAddContentWidget.h"
#include "SVoxelAddContentTile.h"
#include "VoxelExampleContentManager.h"

#include "SPrimaryButton.h"
#include "SWidgetCarouselWithNavigation.h"
#include "Compression/OodleDataCompressionUtil.h"

#include "HttpModule.h"
#include "PlatformHttp.h"
#include "PackageTools.h"
#include "UObject/Linker.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

VOXEL_INITIALIZE_STYLE(AddContentStyle)
{
	const FTextBlockStyle NormalText = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");
	Set("AddContent.Category", FTextBlockStyle(NormalText)
		.SetFont(DEFAULT_FONT("Regular", 12))
		.SetColorAndOpacity(FLinearColor(0.4f, 0.4, 0.4f, 1.0f)));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class SVoxelContentTagCheckBox : public SCheckBox
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(FName, ContentTag)
		SLATE_EVENT(TDelegate<void(bool)>, OnContentTagClicked)
		SLATE_EVENT(FSimpleDelegate, OnContentTagShiftClicked)
		SLATE_ATTRIBUTE(ECheckBoxState, IsChecked)
	};

	void Construct(const FArguments& Args)
	{
		OnContentTagClicked = Args._OnContentTagClicked;
		OnContentTagShiftClicked = Args._OnContentTagShiftClicked;

		const FName ContentTag = Args._ContentTag;
		const FLinearColor TagColor = FVoxelExampleContentManager::GetContentTagColor(ContentTag);

		SCheckBox::Construct(
			SCheckBox::FArguments()
			.Style(FAppStyle::Get(), "ContentBrowser.FilterButton")
			.IsChecked(Args._IsChecked)
			.OnCheckStateChanged_Lambda([this](const ECheckBoxState NewState)
			{
				OnContentTagClicked.ExecuteIfBound(NewState == ECheckBoxState::Checked);
			}));

		SetToolTipText(FText::FromString("Display content with tag: " + FName::NameToDisplayString(ContentTag.ToString(), false) + """.\nUse Shift+Click to exclusively select this tag."));

		SetContent(
			SNew(SBorder)
			.Padding(1.f)
			.BorderImage(FVoxelEditorStyle::GetBrush("Graph.NewAssetDialog.FilterCheckBox.Border"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("ContentBrowser.FilterImage"))
					.ColorAndOpacity_Lambda([=]
					{
						FLinearColor Color = TagColor;
						if (!IsChecked())
						{
							Color.A = 0.1f;
						}

						return Color;
					})
				]
				+ SHorizontalBox::Slot()
				.Padding(MakeAttributeLambda([this]
				{
					return bIsPressed ? FMargin(4.f, 2.f, 3.f, 0.f) : FMargin(4.f, 1.f, 3.f, 1.f);
				}))
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(FText::FromString(FName::NameToDisplayString(ContentTag.ToString(), false)))
					.ColorAndOpacity_Lambda([this]
					{
						if (IsHovered())
						{
							return FLinearColor(0.75f, 0.75f, 0.75f, 1.f);
						}

						return IsChecked() ? FLinearColor::White : FLinearColor::Gray;
					})
					.TextStyle(FVoxelEditorStyle::Get(), "Graph.NewAssetDialog.ActionFilterTextBlock")
				]
			]
		);
	}

	//~ Begin SCheckBox Interface
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		const FReply Reply = SCheckBox::OnMouseButtonUp(MyGeometry, MouseEvent);

		if (FSlateApplication::Get().GetModifierKeys().IsShiftDown() &&
			MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			OnContentTagShiftClicked.ExecuteIfBound();
			return FReply::Handled().ReleaseMouseCapture();
		}

		return Reply;
	}
	//~ End SCheckBox Interface

private:
	TDelegate<void(bool)> OnContentTagClicked;
	FSimpleDelegate OnContentTagShiftClicked;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelAddContentWidget::Construct(const FArguments& InArgs)
{
	GatherContent();

	Filter = MakeVoxelShared<FContentSourceTextFilter>(
		FContentSourceTextFilter::FItemToStringArray::CreateLambda([](const TSharedPtr<FVoxelExampleContent> Item, TArray<FString>& Array)
		{
			Array.Add(Item->Name);
		})
	);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 8.f, 0.f, 4.f))
				[
					SAssignNew(SearchBox, SSearchBox)
					.OnTextChanged(this, &SVoxelAddContentWidget::SearchTextChanged)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
				[
					CreateTagsFilterBox()
				]
				+ SVerticalBox::Slot()
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						CreateContentTilesView()
					]
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(FMargin(18.f, 0.f, 0.f, 0.f))
			.AutoWidth()
			[
				SNew(SSeparator)
				.Orientation(Orient_Vertical)
				.Thickness(2.f)
			]
			+ SHorizontalBox::Slot()
			[
				CreateContentDetails()
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
			.Thickness(2.f)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 16.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			[
				SNew(SPrimaryButton)
				.OnClicked(this, &SVoxelAddContentWidget::AddButtonClicked)
				.Text(INVTEXT("Add to Project"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(8.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
				.TextStyle(FAppStyle::Get(), "DialogButtonText")
				.OnClicked(this, &SVoxelAddContentWidget::CancelButtonClicked)
				.Text(INVTEXT("Cancel"))
			]
		]
	];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FReply SVoxelAddContentWidget::AddButtonClicked()
{
	const TSharedPtr<SWindow> AddContentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	if (!ensure(AddContentWindow))
	{
		return FReply::Handled();
	}

	AddContentWindow->RequestDestroyWindow();

	if (!SelectedContent)
	{
		return FReply::Handled();
	}

	const FVoxelExampleContent& Content = *SelectedContent;

	FString Url = "https://api.voxelplugin.dev/content/download";
	Url += "?name=" + FPlatformHttp::UrlEncode(Content.Name);
	Url += "&version=" + Content.Version;

	const TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb("GET");
	Request->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr, const FHttpResponsePtr Response, const bool bConnectedSuccessfully)
	{
		if (!bConnectedSuccessfully ||
			!Response ||
			Response->GetResponseCode() != 200)
		{
			VOXEL_MESSAGE(Error, "Failed to query content url: {0} {1}", Response->GetResponseCode(), Response->GetContentAsString());
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

		const TSharedPtr<FJsonObject> ContentObject = ParsedValue->AsObject();
		if (!ensure(ContentObject))
		{
			return;
		}

		const FString DownloadUrl = ContentObject->GetStringField(TEXT("url"));
		const int32 DownloadSize = ContentObject->GetNumberField(TEXT("size"));
		const FString Hash = ContentObject->GetStringField(TEXT("hash"));

		LOG_VOXEL(Log, "Downloading %s", *DownloadUrl);

		const TSharedRef<TWeakPtr<IHttpRequest>> SharedWeakRequest = MakeVoxelShared<TWeakPtr<IHttpRequest>>();

		FNotificationInfo Info(FText::FromString("Downloading " + Content.Name));
		Info.bFireAndForget = false;
		Info.ButtonDetails.Add(FNotificationButtonInfo(
			INVTEXT("Cancel"),
			INVTEXT("Cancel download"),
			MakeLambdaDelegate([=]
			{
				if (const TSharedPtr<IHttpRequest> PinnedRequest = SharedWeakRequest->Pin())
				{
					PinnedRequest->CancelRequest();
				}
			}),
			SNotificationItem::CS_None));
		const TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);

		const TSharedRef<IHttpRequest> NewRequest = FHttpModule::Get().CreateRequest();
		*SharedWeakRequest = NewRequest;

		NewRequest->SetURL(DownloadUrl);
		NewRequest->SetVerb("GET");
#if VOXEL_ENGINE_VERSION < 504
		NewRequest->OnRequestProgress().BindLambda([=](const FHttpRequestPtr&, const int32 BytesSent, const int32 BytesReceived)
#else
		NewRequest->OnRequestProgress64().BindLambda([=](const FHttpRequestPtr&, const int64 BytesSent, const int64 BytesReceived)
#endif
		{
			Notification->SetSubText(FText::FromString(FString::Printf(
				TEXT("%.1f/%.1fMB"),
				BytesReceived / double(1 << 20),
				DownloadSize / double(1 << 20))));
		});
		NewRequest->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr, const FHttpResponsePtr& NewResponse, const bool bNewConnectedSuccessfully)
		{
			Notification->ExpireAndFadeout();

			if (!bNewConnectedSuccessfully ||
				!NewResponse ||
				NewResponse->GetResponseCode() != 200)
			{
				VOXEL_MESSAGE(Error, "Failed to download content: {0}", NewResponse->GetResponseCode());
				return;
			}

			const TArray<uint8> Result = NewResponse->GetContent();

			if (!ensure(FSHA1::HashBuffer(Result.GetData(), Result.Num()).ToString() == Hash))
			{
				VOXEL_MESSAGE(Error, "Failed to download {0}: invalid hash", Content.Name);
				return;
			}

			FinalizeDownload(Result);
		});
		NewRequest->ProcessRequest();
	});
	Request->ProcessRequest();

	return FReply::Handled();
}

FReply SVoxelAddContentWidget::CancelButtonClicked()
{
	const TSharedPtr<SWindow> AddContentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	if (!ensure(AddContentWindow))
	{
		return FReply::Handled();
	}

	AddContentWindow->RequestDestroyWindow();

	return FReply::Handled();
}

void SVoxelAddContentWidget::SearchTextChanged(const FText& Text)
{
	Filter->SetRawFilterText(Text);
	UpdateFilteredItems();
}

TSharedRef<SWidget> SVoxelAddContentWidget::CreateTagsFilterBox()
{
	if (Tags.Num() == 0)
	{
		return SNullWidget::NullWidget;
	}

	const auto AllTagsVisible = [this]
	{
		for (const FName ContentTag : Tags)
		{
			if (!VisibleTags.Contains(ContentTag))
			{
				return false;
			}
		}

		return true;
	};

	const TSharedRef<SWrapBox> TagsBox =
		SNew(SWrapBox)
		.UseAllottedSize(true);

	TagsBox->AddSlot()
	.Padding(5.f)
	[
		SNew(SBorder)
        .BorderImage(FVoxelEditorStyle::GetBrush("Graph.NewAssetDialog.FilterCheckBox.Border"))
        .ToolTipText(INVTEXT("Show all"))
        .Padding(3.f)
		[
			SNew(SCheckBox)
            .Style(FVoxelEditorStyle::Get(), "Graph.NewAssetDialog.FilterCheckBox")
            .BorderBackgroundColor_Lambda([=]() -> FSlateColor
			{
			    return AllTagsVisible() ? FLinearColor::White : FSlateColor::UseForeground();
			})
            .IsChecked_Lambda([=]() -> ECheckBoxState
            {
			    return AllTagsVisible() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
            .OnCheckStateChanged_Lambda([=](ECheckBoxState)
			{
			    VisibleTags = AllTagsVisible() ? TSet<FName>() : Tags;
    			UpdateFilteredItems();
			})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(2.f)
				[
					SNew(STextBlock)
                    .Text(INVTEXT("Show all"))
                    .ShadowOffset(0.f)
                    .ColorAndOpacity_Lambda([=]() -> FSlateColor
                    {
					    return AllTagsVisible() ? FLinearColor::Black : FLinearColor::Gray;
					})
                    .TextStyle(FVoxelEditorStyle::Get(), "Graph.NewAssetDialog.ActionFilterTextBlock")
				]
			]
		]
	];

	TArray<FName> SortedTags = Tags.Array();
	SortedTags.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });

	for (const FName ContentTag : SortedTags)
	{
		TagsBox->AddSlot()
		.Padding(2.f)
		[
			SNew(SBorder)
    		.BorderImage(FAppStyle::GetBrush(TEXT("NoBorder")))
			.Padding(3.f)
			[
				SNew(SVoxelContentTagCheckBox)
				.ContentTag(ContentTag)
    			.IsChecked_Lambda([=]
				{
				    return VisibleTags.Contains(ContentTag) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
    			.OnContentTagClicked_Lambda([=](const bool bState)
				{
    				if (bState)
    				{
    					VisibleTags.Add(ContentTag);
    				}
    				else
    				{
    					VisibleTags.Remove(ContentTag);
    				}
    				UpdateFilteredItems();
				})
    			.OnContentTagShiftClicked_Lambda([=]
				{
					VisibleTags = { ContentTag };
    				UpdateFilteredItems();
				})
			]
		];
	}

	return TagsBox;
}

TSharedRef<SWidget> SVoxelAddContentWidget::CreateContentTilesView()
{
	const TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);

	TArray<FName> SortedCategories = Categories.Array();
	SortedCategories.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });

	for (const FName Category : SortedCategories)
	{
		CategoryToFilteredList.Add(Category, {});

		TSharedPtr<STileView<TSharedPtr<FVoxelExampleContent>>> TilesView;

		VerticalBox->AddSlot()
		.AutoHeight()
		[
			SNew(SBox)
			.Visibility_Lambda(MakeWeakPtrLambda(this, [=]
			{
				if (const TArray<TSharedPtr<FVoxelExampleContent>>* FilteredList = CategoryToFilteredList.Find(Category))
				{
					return FilteredList->Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
				}
				return EVisibility::Collapsed;
			}))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromName(Category))
						.TextStyle(FVoxelEditorStyle::Get(), "AddContent.Category")
					]
					+ SHorizontalBox::Slot()
					.Padding(FMargin(14.f, 0.f, 0.f, 0.f))
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Fill)
					[
						SNew(SSeparator)
						.Orientation(Orient_Horizontal)
						.Thickness(1.f)
						.SeparatorImage(FAppStyle::GetBrush("PinnedCommandList.Separator"))
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(TilesView, STileView<TSharedPtr<FVoxelExampleContent>>)
					.ListItemsSource(&CategoryToFilteredList[Category])
					.OnGenerateTile(this, &SVoxelAddContentWidget::CreateContentSourceIconTile)
					.OnSelectionChanged_Lambda([this](const TSharedPtr<FVoxelExampleContent> NewSelection, const ESelectInfo::Type SelectInfo)
					{
						if (SelectInfo == ESelectInfo::Direct)
						{
							return;
						}

						if (SelectedContent)
						{
							if (const TSharedPtr<STileView<TSharedPtr<FVoxelExampleContent>>> View = CategoryToTilesView.FindRef(SelectedContent->Category))
							{
								View->ClearSelection();
							}
						}

						const TSharedPtr<FVoxelExampleContent> OldContent = SelectedContent;
						SelectedContent = NewSelection;

						UpdateContentTags(OldContent);
					})
					.ClearSelectionOnClick(false)
					.ItemAlignment(EListItemAlignment::LeftAligned)
					.ItemWidth(102.f)
					.ItemHeight(153.f)
					.SelectionMode(ESelectionMode::Single)
				]
			]
		];

		CategoryToTilesView.Add(Category, TilesView);
	}

	UpdateFilteredItems();

	return VerticalBox;
}

TSharedRef<SWidget> SVoxelAddContentWidget::CreateContentDetails()
{
	ON_SCOPE_EXIT
	{
		UpdateContentTags(nullptr);
	};

	return
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		.Padding(0.f, 0.f, 0.f, 5.f)
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFitX)
			[
				SNew(SImage)
				.Image_Lambda([=]() -> const FSlateBrush*
				{
					if (!SelectedContent)
					{
						return nullptr;
					}

					const ISlateBrushSource* BrushSource = SelectedContent->Image->Get();
					if (!BrushSource)
					{
						return nullptr;
					}
					return BrushSource->GetSlateBrush();
				})
			]
		]
		+ SScrollBox::Slot()
		.Padding(10.f, 0.f, 0.f, 5.f)
		[
			SNew(STextBlock)
			.TextStyle(FAppStyle::Get(), "DialogButtonText")
			.Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
			.ColorAndOpacity(FStyleColors::ForegroundHover)
			.Text_Lambda([=]() -> FText
			{
				if (!SelectedContent)
				{
					return {};
				}

				return FText::FromString(SelectedContent->DisplayName);
			})
			.AutoWrapText(true)
		]
		+ SScrollBox::Slot()
		.Padding(10.f, 0.f, 0.f, 5.f)
		[
			SAssignNew(SelectedContentTagsBox, SWrapBox)
			.UseAllottedSize(true)
		]
		+ SScrollBox::Slot()
		.Padding(10.f, 0.f, 0.f, 5.f)
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text_Lambda([=]() -> FText
			{
				if (!SelectedContent)
				{
					return {};
				}

				return FText::FromString(SelectedContent->Description);
			})
		];
}

TSharedRef<ITableRow> SVoxelAddContentWidget::CreateContentSourceIconTile(const TSharedPtr<FVoxelExampleContent> Content, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return
		SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		.Style(FAppStyle::Get(), "ProjectBrowser.TableRow")
		.Padding(2.f)
		[
			SNew(SVoxelAddContentTile)
			.ContentTags(Content->Tags)
			.VisibleTags_Lambda([this]
			{
				return VisibleTags;
			})
			.Image_Lambda([=]() -> const FSlateBrush*
			{
				const ISlateBrushSource* BrushSource = Content->Thumbnail->Get();
				if (!BrushSource)
				{
					return nullptr;
				}
				return BrushSource->GetSlateBrush();
			})
			.DisplayName(FText::FromString(Content->DisplayName))
			.IsSelected_Lambda([this, WeakContent = MakeWeakPtr(Content)]
			{
				return WeakContent == SelectedContent;
			})
		];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelAddContentWidget::GatherContent()
{
	ContentsSourceList = FVoxelExampleContentManager::Get().GetExamples();

	for (const TSharedPtr<FVoxelExampleContent>& Content : ContentsSourceList)
	{
		if (!Categories.Contains(Content->Category))
		{
			Categories.Add(Content->Category);
		}

		for (const FName ContentTag : Content->Tags)
		{
			if (!Tags.Contains(ContentTag))
			{
				Tags.Add(ContentTag);
			}
		}
	}

	VisibleTags = Tags;
}

void SVoxelAddContentWidget::UpdateFilteredItems()
{
	const TArray<TSharedPtr<FVoxelExampleContent>> FilteredList = ContentsSourceList.FilterByPredicate([&](const TSharedPtr<FVoxelExampleContent>& Content)
	{
		if (!Filter->PassesFilter(Content))
		{
			return false;
		}

		if (Content->Tags.Num() == 0)
		{
			return true;
		}

		for (const FName ContentTag : Content->Tags)
		{
			if (VisibleTags.Contains(ContentTag))
			{
				return true;
			}
		}

		return false;
	});

	// Clear selection, since no entries to select
	if (FilteredList.Num() == 0 &&
		SelectedContent)
	{
		if (const TSharedPtr<STileView<TSharedPtr<FVoxelExampleContent>>> TilesView = CategoryToTilesView.FindRef(SelectedContent->Category))
		{
			TilesView->SetSelection(nullptr, ESelectInfo::OnMouseClick);
		}
	}

	for (const FName Category : Categories)
	{
		const TSharedPtr<STileView<TSharedPtr<FVoxelExampleContent>>>& TilesView = CategoryToTilesView.FindRef(Category);
		TArray<TSharedPtr<FVoxelExampleContent>>* List = CategoryToFilteredList.Find(Category);
		if (!ensure(TilesView) ||
			!ensure(List))
		{
			continue;
		}

		*List = FilteredList.FilterByPredicate([&](const TSharedPtr<FVoxelExampleContent>& Content)
		{
			return Category == Content->Category;
		});

		TilesView->RequestListRefresh();

		if (List->Num() == 0)
		{
			// Clear tiles view selection, so that all tiles could be reselected, but don't update content data
			TilesView->ClearSelection();
			continue;
		}

		if (List->Contains(SelectedContent))
		{
			TilesView->SetSelection(SelectedContent, ESelectInfo::OnMouseClick);
		}
		else if (
			!SelectedContent ||
			Category == SelectedContent->Category)
		{
			TilesView->SetSelection((*List)[0], ESelectInfo::OnMouseClick);
		}
	}
}

void SVoxelAddContentWidget::UpdateContentTags(const TSharedPtr<FVoxelExampleContent>& OldContent) const
{
	if (!SelectedContentTagsBox ||
		OldContent == SelectedContent)
	{
		return;
	}

	SelectedContentTagsBox->ClearChildren();

	if (!SelectedContent)
	{
		return;
	}

	TArray<FName> ContentTags = SelectedContent->Tags.Array();
	ContentTags.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });

	for (const FName ContentTag : ContentTags)
	{
		SelectedContentTagsBox->AddSlot()
		.Padding(3.f)
		[
			SNew(SBorder)
			.Padding(0.f)
			.BorderImage(FVoxelEditorStyle::GetBrush("Graph.NewAssetDialog.FilterCheckBox.Border"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("ContentBrowser.FilterImage"))
					.ColorAndOpacity(FVoxelExampleContentManager::GetContentTagColor(ContentTag))
				]
				+ SHorizontalBox::Slot()
				.Padding(4.f, 1.f, 4.f, 0.f)
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(FText::FromString(FName::NameToDisplayString(ContentTag.ToString(), false)))
					.ColorAndOpacity(FLinearColor::White)
					.TextStyle(FVoxelEditorStyle::Get(), "Graph.NewAssetDialog.ActionFilterTextBlock")
				]
			]
		];
	}
}

void SVoxelAddContentWidget::FinalizeDownload(const TArray<uint8>& InData) const
{
	TArray<uint8> UncompressedData;
	if (!FOodleCompressedArray::DecompressToTArray(UncompressedData, InData))
	{
		VOXEL_MESSAGE(Error, "Failed to download content: decompression failed");
		return;
	}

	TMap<FString, TVoxelArray64<uint8>> Files;
	const FString ZipError = FVoxelUtilities::Unzip(UncompressedData, Files);
	if (!ZipError.IsEmpty() ||
		!ensure(Files.Num() > 0))
	{
		VOXEL_MESSAGE(Error, "Failed to download content: unzip failed: {0}", ZipError);
		return;
	}

	TSet<FString> PackageNames;
	TSet<FString> ExistingPackageNames;
	for (const auto& It : Files)
	{
		if (!ensure(It.Key.StartsWith("Content")))
		{
			VOXEL_MESSAGE(Error, "Failed to download content: invalid path {0}", It.Key);
			return;
		}

		const FString Path = FPaths::ProjectDir() / It.Key;

		FString PackageName;
		if (!ensure(FPackageName::TryConvertFilenameToLongPackageName(Path, PackageName)))
		{
			continue;
		}

		if (FPaths::FileExists(Path))
		{
			TArray<uint8> ExistingFileData;
			if (FFileHelper::LoadFileToArray(ExistingFileData, *Path))
			{
				if (FVoxelUtilities::Equal(It.Value, ExistingFileData))
				{
					continue;
				}
			}

			ExistingPackageNames.Add(PackageName);
		}

		PackageNames.Add(PackageName);
	}

	if (PackageNames.Num() == 0 &&
		Files.Num() > 0)
	{
		VOXEL_MESSAGE(Info, "The downloaded files are the same as the existing ones. No files were changed.");
		return;
	}

	if (ExistingPackageNames.Num() > 0)
	{
		FString Text = "The following assets already exist:\n";
		for (const FString& Path : ExistingPackageNames)
		{
			Text += Path + "\n";
		}
		Text += "\nDo you want to overwrite them?";

		const EAppReturnType::Type Return = FMessageDialog::Open(EAppMsgType::YesNoCancel, FText::FromString(Text));
		if (Return == EAppReturnType::Cancel)
		{
			return;
		}
		if (Return == EAppReturnType::No)
		{
			for (const FString& PackageName : ExistingPackageNames)
			{
				ensure(PackageNames.Remove(PackageName));
			}
		}
	}

	TArray<UPackage*> LoadedPackages;
	for (const FString& PackageName : PackageNames)
	{
		UPackage* Package = FindPackage(nullptr, *PackageName);
		if (!Package)
		{
			continue;
		}

		LoadedPackages.Add(Package);
	}

	for (UPackage* Package : LoadedPackages)
	{
		if (!Package->IsFullyLoaded())
		{
			FlushAsyncLoading();
			Package->FullyLoad();
		}
		ResetLoaders(Package);
	}

	for (const auto& It : Files)
	{
		const FString Path = FPaths::ProjectDir() / It.Key;

		FString PackageName;
		if (!ensure(FPackageName::TryConvertFilenameToLongPackageName(Path, PackageName)))
		{
			continue;
		}

		if (!PackageNames.Contains(PackageName))
		{
			continue;
		}

		if (!FFileHelper::SaveArrayToFile(It.Value, *Path))
		{
			VOXEL_MESSAGE(Error, "Failed to write {0}", Path);
		}
	}

	if (LoadedPackages.Num() > 0)
	{
		FText ErrorMessage;
		if (!UPackageTools::ReloadPackages(LoadedPackages, ErrorMessage, EReloadPackagesInteractionMode::AssumePositive))
		{
			VOXEL_MESSAGE(Error, "Failed to reload packages: {0}", ErrorMessage);
		}
	}

	VOXEL_MESSAGE(Info, "Content added");

	FVoxelUtilities::DelayedCall([=]
	{
		TArray<UObject*> AssetObjects;
		for (const auto& It : Files)
		{
			FString PackageName;
			if (!ensure(FPackageName::TryConvertFilenameToLongPackageName(
				FPaths::ProjectDir() / It.Key,
				PackageName)))
			{
				continue;
			}

			const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

			TArray<FAssetData> AssetDatas;
			AssetRegistryModule.Get().GetAssetsByPackageName(*PackageName, AssetDatas);

			for (const FAssetData& AssetData : AssetDatas)
			{
				if (UObject* AssetObject = AssetData.GetAsset())
				{
					AssetObjects.Add(AssetObject);
				}
			}
		}

		if (!ensure(AssetObjects.Num() > 0))
		{
			return;
		}

		for (UObject* Object : AssetObjects)
		{
			if (Object->IsA<UWorld>())
			{
				AssetObjects = { Object };
				GEditor->SyncBrowserToObjects({ AssetObjects });
				return;
			}
		}

		GEditor->SyncBrowserToObjects(AssetObjects);
	}, 1.f);
}