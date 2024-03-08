// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelCurveEditor.h"

#include "VoxelCurveModel.h"
#include "SVoxelCurvePanel.h"
#include "Widgets/SVoxelCurveThumbnail.h"

#include "AssetToolsModule.h"
#include "CurveEditorCommands.h"
#include "ContentBrowserModule.h"
#include "SGridLineSpacingList.h"
#include "Slate/SRetainerWidget.h"
#include "Factories/CurveFactory.h"
#include "IContentBrowserSingleton.h"

void SVoxelCurveEditor::Construct(const FArguments& InArgs)
{
	if (!ensure(InArgs._Handle))
	{
		ChildSlot
		[
			SNullWidget::NullWidget
		];
		return;
	}

	CurveModel = MakeShared<FVoxelCurveModel>(InArgs._Handle, FSimpleDelegate::CreateSP(this, &SVoxelCurveEditor::InvalidateRetainer));
	CommandList = MakeShared<FUICommandList>();

	CurveModel->BindCommands(CommandList);

	SAssignNew(CurvePanel, SVoxelCurvePanel)
	.CurveModel(CurveModel)
	.CommandList(CommandList)
	.CurveTint(InArgs._CurveTint)
	.KeysTint(InArgs._KeysTint)
	.TangentsTint(InArgs._TangentsTint)
	.Invalidate(this, &SVoxelCurveEditor::InvalidateRetainer)
	.Visibility(EVisibility::Visible);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeTopToolbar()
		]
		+ SVerticalBox::Slot()
		.MaxHeight(300.f)
		[
			SAssignNew(RetainerWidget, SRetainerWidget)
			.RenderOnPhase(false)
			.RenderOnInvalidation(true)
#if VOXEL_ENGINE_VERSION < 504
			.RenderWithLocalTransform(false)
#endif
			[
				CurvePanel.ToSharedRef()
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MakeBottomToolbar()
		]
	];
}

FReply SVoxelCurveEditor::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (CommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SVoxelCurveEditor::RefreshCurve() const
{
	CurveModel->RefreshCurve();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelCurveEditor::InvalidateRetainer() const
{
	if (RetainerWidget)
	{
		RetainerWidget->RequestRender();
	}
}

TSharedRef<SWidget> SVoxelCurveEditor::MakeTopToolbar()
{
	FToolBarBuilder ToolBarBuilder(CommandList, FMultiBoxCustomization::None, nullptr, true);
	ToolBarBuilder.SetStyle(&FAppStyle::Get(), "SlimToolbar");

	ToolBarBuilder.BeginSection("SaveLoad");
	{
		ToolBarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateLambda([this]
			{
				FAssetPickerConfig AssetPickerConfig;
				AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SVoxelCurveEditor::CopySelectedCurve);
				AssetPickerConfig.bAllowNullSelection = false;
				AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
				AssetPickerConfig.Filter.ClassPaths.Add(UCurveFloat::StaticClass()->GetClassPathName());

				const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

				return
					SNew(SBox)
					.WidthOverride(300.0f)
					.HeightOverride(480.f)
					[
						ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
					];
			}),
			INVTEXT("Import"),
			INVTEXT("Import data from another Curve asset"),
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "MeshPaint.Import.Small"));

		ToolBarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateSP(this, &SVoxelCurveEditor::SaveCurveToAsset)),
			{},
			INVTEXT("Save"),
			INVTEXT("Save data to new Curve asset"),
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "MeshPaint.Save.Small"));
	}
	ToolBarBuilder.EndSection();

	ToolBarBuilder.BeginSection("Adjustment");
	{
		ToolBarBuilder.AddToolBarButton(
			FUIAction(
					FExecuteAction::CreateSP(CurvePanel.Get(), &SVoxelCurvePanel::ToggleInputSnapping),
					{},
					FCanExecuteAction::CreateSP(CurvePanel.Get(), &SVoxelCurvePanel::IsInputSnappingEnabled)),
				{},
				INVTEXT("Input Snapping"),
				INVTEXT("Toggle Time Snapping"),
				FSlateIcon(FAppStyle::Get().GetStyleSetName(), "GenericCurveEditor.ToggleInputSnapping"),
				EUserInterfaceActionType::ToggleButton);

		ToolBarBuilder.AddComboButton(
			FUIAction(FExecuteAction(), FCanExecuteAction::CreateSP(CurvePanel.Get(), &SVoxelCurvePanel::IsInputSnappingEnabled)),
			FOnGetContent::CreateSP(this, &SVoxelCurveEditor::MakeSnapMenu, true),
			INVTEXT("Time Snapping"),
			INVTEXT("Choose the spacing between horizontal grid lines."),
			TAttribute<FSlateIcon>(),
			true);

		ToolBarBuilder.AddToolBarButton(
			FUIAction(
					FExecuteAction::CreateSP(CurvePanel.Get(), &SVoxelCurvePanel::ToggleOutputSnapping),
					{},
					FCanExecuteAction::CreateSP(CurvePanel.Get(), &SVoxelCurvePanel::IsOutputSnappingEnabled)),
				{},
				INVTEXT("Output Snapping"),
				INVTEXT("Toggle Value Snapping"),
				FSlateIcon(FAppStyle::Get().GetStyleSetName(), "GenericCurveEditor.ToggleOutputSnapping"),
				EUserInterfaceActionType::ToggleButton);

		ToolBarBuilder.AddComboButton(
			FUIAction(FExecuteAction(), FCanExecuteAction::CreateSP(CurvePanel.Get(), &SVoxelCurvePanel::IsOutputSnappingEnabled)),
			FOnGetContent::CreateSP(this, &SVoxelCurveEditor::MakeSnapMenu, false),
			INVTEXT("Value Snapping"),
			INVTEXT("Choose the spacing between vertical grid lines."),
			TAttribute<FSlateIcon>(),
			true);

		TAttribute<FSlateIcon> AxisSnappingModeIcon;
		AxisSnappingModeIcon.Bind(TAttribute<FSlateIcon>::FGetter::CreateLambda(MakeWeakPtrLambda(CurvePanel, [this] {
			switch (CurvePanel->GetAxisSnapping())
			{
			case EAxisList::Type::X: return FSlateIcon(FAppStyle::Get().GetStyleSetName(), "GenericCurveEditor.SetAxisSnappingHorizontal");
			case EAxisList::Type::Y: return FSlateIcon(FAppStyle::Get().GetStyleSetName(), "GenericCurveEditor.SetAxisSnappingVertical");
			default: return FSlateIcon(FAppStyle::Get().GetStyleSetName(), "GenericCurveEditor.SetAxisSnappingNone");
			}
		})));

		ToolBarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateSP(this, &SVoxelCurveEditor::MakeAxisSnappingMenu),
			INVTEXT("Axis Snapping"),
			INVTEXT("Choose which axes movement tools are locked to."),
			AxisSnappingModeIcon);
	}
	ToolBarBuilder.EndSection();

	ToolBarBuilder.BeginSection("Tangents");
	{
		const auto AddButton = [&](const TSharedPtr<FUICommandInfo>& Command)
		{
			ToolBarBuilder.AddToolBarButton(Command, {}, Command->GetLabel(), Command->GetDescription(), Command->GetIcon());
		};

		UE_503_ONLY(AddButton(FCurveEditorCommands::Get().InterpolationCubicSmartAuto));
		AddButton(FCurveEditorCommands::Get().InterpolationCubicAuto);
		AddButton(FCurveEditorCommands::Get().InterpolationCubicUser);
		AddButton(FCurveEditorCommands::Get().InterpolationCubicBreak);
		AddButton(FCurveEditorCommands::Get().InterpolationLinear);
		AddButton(FCurveEditorCommands::Get().InterpolationConstant);
	}
	ToolBarBuilder.EndSection();

	ToolBarBuilder.BeginSection("Templates");
	{
		ToolBarBuilder.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateSP(this, &SVoxelCurveEditor::MakeTemplatesMenu),
			INVTEXT("Templates"),
			INVTEXT("Choose curve template."),
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "GenericCurveEditor.TabIcon"));
	}
	ToolBarBuilder.EndSection();

	ToolBarBuilder.BeginSection("Settings");
	{
		ToolBarBuilder.AddToolBarButton(
			FUIAction(
					FExecuteAction::CreateSP(CurveModel.Get(), &FVoxelCurveModel::ExtendByTangents),
					{},
					FCanExecuteAction::CreateSP(CurveModel.Get(), &FVoxelCurveModel::IsExtendByTangentsEnabled)),
				{},
				INVTEXT("Expand view by Tangents"),
				INVTEXT("Expand view if Tangent exceeds bounds"),
				FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Curve.ZoomToFitVertical"),
				EUserInterfaceActionType::ToggleButton);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

TSharedRef<SWidget> SVoxelCurveEditor::MakeBottomToolbar()
{
	FToolBarBuilder ToolBarBuilder(nullptr, FMultiBoxCustomization::None, nullptr, true);
	ToolBarBuilder.SetStyle(&FAppStyle::Get(), "SlimToolbar");

	ToolBarBuilder.BeginSection("Key Details");
	{
		ToolBarBuilder.AddToolBarWidget(
			SNew(SBox)
			.MinDesiredWidth(125.f)
			.MaxDesiredWidth(125.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SVoxelDetailText)
					.Text(INVTEXT("X: "))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SOverlay)
					.IsEnabled(CurveModel.Get(), &FVoxelCurveModel::HasSelectedKeys)
					+ SOverlay::Slot()
					[
						SNew(SNumericEntryBox<float>)
						.Visibility(CurveModel.Get(), &FVoxelCurveModel::HasSelectedKeys, false)
						.AllowSpin(false)
						.UndeterminedString(INVTEXT("Multiple Values"))
						.Value(CurveModel.Get(), &FVoxelCurveModel::GetSelectedKeysInput)
						.OnValueCommitted(CurveModel.Get(), &FVoxelCurveModel::SetSelectedKeysInput)
					]
					+ SOverlay::Slot()
					[
						SNew(SEditableTextBox)
						.Visibility(CurveModel.Get(), &FVoxelCurveModel::HasSelectedKeys, true)
					]
				]
				]);
		ToolBarBuilder.AddToolBarWidget(
			SNew(SBox)
			.MinDesiredWidth(125.f)
			.MaxDesiredWidth(125.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SVoxelDetailText)
					.Text(INVTEXT("Y: "))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SOverlay)
					.IsEnabled(CurveModel.Get(), &FVoxelCurveModel::HasSelectedKeys)
					+ SOverlay::Slot()
					[
						SNew(SNumericEntryBox<float>)
						.Visibility(CurveModel.Get(), &FVoxelCurveModel::HasSelectedKeys, false)
						.AllowSpin(false)
						.UndeterminedString(INVTEXT("Multiple Values"))
						.Value(CurveModel.Get(), &FVoxelCurveModel::GetSelectedKeysOutput)
						.OnValueCommitted(CurveModel.Get(), &FVoxelCurveModel::SetSelectedKeysOutput)
					]
					+ SOverlay::Slot()
					[
						SNew(SEditableTextBox)
						.Visibility(CurveModel.Get(), &FVoxelCurveModel::HasSelectedKeys, true)
					]
				]
			]);
	}
	ToolBarBuilder.EndSection();

	return ToolBarBuilder.MakeWidget();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelCurveEditor::CopySelectedCurve(const FAssetData& AssetData) const
{
	ON_SCOPE_EXIT
	{
		FSlateApplication::Get().DismissAllMenus();
	};

	const UCurveFloat* CurveAsset = Cast<UCurveFloat>(AssetData.GetAsset());
	if (!ensure(CurveAsset))
	{
		return;
	}

	CurveModel->SetCurve(CurveAsset->FloatCurve);
}

void SVoxelCurveEditor::SaveCurveToAsset() const
{
	const FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();

	UCurveFloatFactory* Factory = NewObject<UCurveFloatFactory>();
	UCurveFloat* NewCurve = Cast<UCurveFloat>(AssetToolsModule.Get().CreateAssetWithDialog(UCurveFloat::StaticClass(), Factory));
	if (!NewCurve)
	{
		return;
	}

	NewCurve->FloatCurve = *CurveModel->Curve;
	NewCurve->PostEditChange();
}

TSharedRef<SWidget> SVoxelCurveEditor::MakeSnapMenu(const bool bInput)
{
	TArray<SGridLineSpacingList::FNamedValue> SpacingAmounts;
	SpacingAmounts.Add(SGridLineSpacingList::FNamedValue(0.001f, INVTEXT("0.001"), INVTEXT("Set snap to 1/1000th")));
	SpacingAmounts.Add(SGridLineSpacingList::FNamedValue(0.01f, INVTEXT("0.01"), INVTEXT("Set snap to 1/100th")));
	SpacingAmounts.Add(SGridLineSpacingList::FNamedValue(0.1f, INVTEXT("0.1"), INVTEXT("Set grid spacing to 1/10th")));
	SpacingAmounts.Add(SGridLineSpacingList::FNamedValue(0.5f, INVTEXT("0.5"), INVTEXT("Set grid spacing to 1/2")));
	SpacingAmounts.Add(SGridLineSpacingList::FNamedValue(1.0f, INVTEXT("1"), INVTEXT("Set grid spacing to 1")));
	SpacingAmounts.Add(SGridLineSpacingList::FNamedValue(2.0f, INVTEXT("2"), INVTEXT("Set grid spacing to 2")));
	SpacingAmounts.Add(SGridLineSpacingList::FNamedValue(5.0f, INVTEXT("5"), INVTEXT("Set grid spacing to 5")));
	SpacingAmounts.Add(SGridLineSpacingList::FNamedValue(10.0f, INVTEXT("10"), INVTEXT("Set grid spacing to 10")));
	SpacingAmounts.Add(SGridLineSpacingList::FNamedValue(50.0f, INVTEXT("50"), INVTEXT("Set grid spacing to 50")));
	SpacingAmounts.Add(SGridLineSpacingList::FNamedValue(100.0f, INVTEXT("100"), INVTEXT("Set grid spacing to 100")));
	SpacingAmounts.Add(SGridLineSpacingList::FNamedValue(TOptional<float>(), INVTEXT("Automatic"), INVTEXT("Set grid spacing to automatic")));

	TSharedRef<SWidget> OutputSnapWidget =
		SNew(SGridLineSpacingList)
		.DropDownValues(SpacingAmounts)
		.MinDesiredValueWidth(60)
		.Value(CurvePanel.Get(), &SVoxelCurvePanel::GetGridSpacing, bInput)
		.OnValueChanged(CurvePanel.Get(), &SVoxelCurvePanel::SetGridSpacing, bInput)
		.HeaderText(bInput ? INVTEXT("Horizontal Spacing") : INVTEXT("Vertical spacing"));

	return OutputSnapWidget;
}

TSharedRef<SWidget> SVoxelCurveEditor::MakeAxisSnappingMenu() const
{
	FMenuBuilder MenuBuilder(true, CommandList);

	MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().SetAxisSnappingNone);
	MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().SetAxisSnappingHorizontal);
	MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().SetAxisSnappingVertical);

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SVoxelCurveEditor::MakeTemplatesMenu()
{
	FMenuBuilder MenuBuilder(true, CommandList);

#if 0 // TODO ERLANDAS
	for (const FNiagaraCurveTemplate& CurveTemplate : GetDefault<UNiagaraEditorSettings>()->GetCurveTemplates())
	{
		UCurveFloat* FloatCurveAsset = Cast<UCurveFloat>(CurveTemplate.CurveAsset.TryLoad());
		if (!FloatCurveAsset)
		{
			continue;
		}

		MenuBuilder.AddWidget(
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "Menu.Button")
			.OnClicked(this, &SVoxelCurveEditor::OnChangeCurveTemplate, TWeakObjectPtr<UCurveFloat>(FloatCurveAsset))
			.ToolTipText(FText::FromString(FName::NameToDisplayString(FloatCurveAsset->GetName(), false) + "\nClick to apply this template to the selected curves."))
			.ContentPadding(3.f)
			.Content()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(2.f, 0.f, 6.f, 0.f)
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SVoxelCurveThumbnail, &FloatCurveAsset->FloatCurve)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.Padding(2.f, 0.f, 6.f, 0.f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(FAppStyle::Get(), "Menu.Label")
					.Text(FText::FromString(FName::NameToDisplayString(FloatCurveAsset->GetName(), false)))
				]
			],
			INVTEXT(""));
	}
#endif

	return MenuBuilder.MakeWidget();
}

FReply SVoxelCurveEditor::OnChangeCurveTemplate(const TWeakObjectPtr<UCurveFloat> WeakFloatCurveAsset) const
{
	const UCurveFloat* FloatCurveAsset = WeakFloatCurveAsset.Get();
	if (!ensure(FloatCurveAsset))
	{
		return FReply::Handled();
	}

	CurveModel->SetCurve(FloatCurveAsset->FloatCurve);

	return FReply::Handled();
}