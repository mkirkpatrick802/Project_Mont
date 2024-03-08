// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Material/SVoxelMaterialDefinitionParameters.h"
#include "Material/VoxelMaterialDefinitionToolkit.h"
#include "Material/VoxelMaterialDefinitionParameterSchemaAction.h"
#include "Material/VoxelMaterialDefinitionParameterDragDropAction.h"
#include "SGraphActionMenu.h"

#if VOXEL_ENGINE_VERSION >= 503
#include "GraphActionNode.h"
#else
#include "GraphEditor/Private/GraphActionNode.h"
#endif

void SVoxelMaterialDefinitionParameters::Construct(const FArguments& Args)
{
	check(Args._Toolkit.IsValid());
	WeakToolkit = Args._Toolkit;

	Args._Toolkit->Asset->OnParametersChanged.Add(MakeWeakPtrDelegate(this, [=]
	{
		bRefreshQueued = true;
	}));

	CommandList->MapAction(FGenericCommands::Get().Delete, MakeWeakPtrDelegate(this, [this]
	{
		const TSharedPtr<FVoxelMaterialDefinitionToolkit> Toolkit = GetToolkit();
		if (!ensure(Toolkit))
		{
			return;
		}
		UVoxelMaterialDefinition* MaterialDefinition = Toolkit->Asset;

		const TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> Action = GetSelectedAction();
		if (!Action)
		{
			return;
		}

		{
			const FVoxelTransaction Transaction(MaterialDefinition, "Delete parameter");
			ensure(MaterialDefinition->GuidToMaterialParameter.Remove(Action->Guid));
		}

		Toolkit->SelectParameter({}, false, true);
	}));

	CommandList->MapAction(FGenericCommands::Get().Duplicate, MakeWeakPtrDelegate(this, [this]
	{
		const TSharedPtr<FVoxelMaterialDefinitionToolkit> Toolkit = GetToolkit();
		if (!ensure(Toolkit))
		{
			return;
		}
		UVoxelMaterialDefinition* MaterialDefinition = Toolkit->Asset;

		const TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> Action = GetSelectedAction();
		if (!Action)
		{
			return;
		}

		const FVoxelMaterialDefinitionParameter* Parameter = MaterialDefinition->GuidToMaterialParameter.Find(Action->Guid);
		if (!ensure(Parameter))
		{
			return;
		}

		const FGuid NewGuid = FGuid::NewGuid();

		{
			const FVoxelTransaction Transaction(MaterialDefinition, "Duplicate parameter");
			MaterialDefinition->GuidToMaterialParameter.Add(NewGuid, *Parameter);
		}

		Toolkit->SelectParameter(NewGuid, true, true);
	}));

	CommandList->MapAction(FGenericCommands::Get().Rename, MakeWeakPtrDelegate(this, [this]
	{
		MembersMenu->OnRequestRenameOnActionNode();
	}),
	MakeWeakPtrDelegate(this, [this]
	{
		const TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> Action = GetSelectedAction();
		if (!Action)
		{
			return false;
		}

		return MembersMenu->CanRequestRenameOnActionNode();
	}));

	CommandList->MapAction(FGenericCommands::Get().Copy, MakeWeakPtrDelegate(this, [this]
	{
		const TSharedPtr<FVoxelMaterialDefinitionToolkit> Toolkit = GetToolkit();
		if (!ensure(Toolkit))
		{
			return;
		}
		UVoxelMaterialDefinition* MaterialDefinition = Toolkit->Asset;

		const TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> Action = GetSelectedAction();
		if (!Action)
		{
			return;
		}

		const FVoxelMaterialDefinitionParameter* Parameter = Toolkit->Asset->GuidToMaterialParameter.Find(Action->Guid);
		if (!ensure(Parameter))
		{
			return;
		}

		FString Text;
		FVoxelMaterialDefinitionParameter::StaticStruct()->ExportText(
			Text,
			Parameter,
			Parameter,
			nullptr,
			0,
			nullptr);

		Text = "SVoxelMaterialDefinitionParameters" + Text;
		FPlatformApplicationMisc::ClipboardCopy(*Text);
	}));

	CommandList->MapAction(FGenericCommands::Get().Cut, MakeWeakPtrDelegate(this, [this]
	{
		const TSharedPtr<FVoxelMaterialDefinitionToolkit> Toolkit = GetToolkit();
		if (!ensure(Toolkit))
		{
			return;
		}
		UVoxelMaterialDefinition* MaterialDefinition = Toolkit->Asset;

		const TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> Action = GetSelectedAction();
		if (!Action)
		{
			return;
		}

		const FVoxelMaterialDefinitionParameter* Parameter = MaterialDefinition->GuidToMaterialParameter.Find(Action->Guid);
		if (!ensure(Parameter))
		{
			return;
		}

		FString Text;
		FVoxelMaterialDefinitionParameter::StaticStruct()->ExportText(
			Text,
			Parameter,
			Parameter,
			nullptr,
			0,
			nullptr);

		Text = "SVoxelMaterialDefinitionParameters" + Text;
		FPlatformApplicationMisc::ClipboardCopy(*Text);

		{
			const FVoxelTransaction Transaction(MaterialDefinition, "Cut parameter");
			ensure(MaterialDefinition->GuidToMaterialParameter.Remove(Action->Guid));
		}

		Toolkit->SelectParameter({}, false, true);
	}));

	CommandList->MapAction(FGenericCommands::Get().Paste, MakeWeakPtrDelegate(this, [this]
	{
		const TSharedPtr<FVoxelMaterialDefinitionToolkit> Toolkit = GetToolkit();
		if (!ensure(Toolkit))
		{
			return;
		}
		UVoxelMaterialDefinition* MaterialDefinition = Toolkit->Asset;

		FString ClipboardText;
		FPlatformApplicationMisc::ClipboardPaste(ClipboardText);

		if (!ClipboardText.RemoveFromStart("SVoxelMaterialDefinitionParameters"))
		{
			return;
		}

		FVoxelMaterialDefinitionParameter NewParameter;
		if (!FVoxelUtilities::TryImportText(ClipboardText, NewParameter))
		{
			return;
		}

		NewParameter.Category = GetPasteCategory();
		const FGuid Guid = FGuid::NewGuid();

		{
			const FVoxelTransaction Transaction(MaterialDefinition, "Paste parameter");
			MaterialDefinition->GuidToMaterialParameter.Add(Guid, NewParameter);
		}

		Toolkit->SelectParameter(Guid, true, true);
	}),
	MakeWeakPtrDelegate(this, [this]
	{
		FString ClipboardText;
		FPlatformApplicationMisc::ClipboardPaste(ClipboardText);

		return ClipboardText.StartsWith("SVoxelMaterialDefinitionParameters");
	}));

	SAssignNew(FilterBox, SSearchBox)
	.OnTextChanged_Lambda([this](const FText&)
	{
		MembersMenu->GenerateFilteredItems(false);
	});

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.Padding(4.0f)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				FilterBox.ToSharedRef()
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SAssignNew(MembersMenu, SGraphActionMenu, false)
			.AlphaSortItems(false)
			.UseSectionStyling(true)
			.AutoExpandActionMenu(true)
			.OnCollectStaticSections_Lambda([=](TArray<int32>& StaticSectionIds)
			{
				StaticSectionIds.Add(1);
			})
			.OnGetSectionTitle_Lambda([](const int32)
			{
				return INVTEXT("Parameters");
			})
			.OnGetSectionWidget_Lambda([=](TSharedRef<SWidget> RowWidget, const int32 SectionId)
			{
				return
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.OnClicked_Lambda([this]
					{
						OnAddNewMember();
						return FReply::Handled();
					})
					.ContentPadding(FMargin(1, 0))
					.ToolTipText(INVTEXT("Add parameter"))
					[
						SNew(SImage)
						.Image(FAppStyle::Get().GetBrush("Icons.PlusCircle"))
						.ColorAndOpacity(FSlateColor::UseForeground())
					];
			})
			.OnContextMenuOpening(this, &SVoxelMaterialDefinitionParameters::OnContextMenuOpening)
			.OnCollectAllActions_Lambda([=](FGraphActionListBuilderBase& OutAllActions)
			{
				const TSharedPtr<FVoxelMaterialDefinitionToolkit> Toolkit = GetToolkit();
				if (!ensure(Toolkit))
				{
					return;
				}
				UVoxelMaterialDefinition* MaterialDefinition = Toolkit->Asset;

				for (const auto& It : MaterialDefinition->GuidToMaterialParameter)
				{
					const FVoxelMaterialDefinitionParameter& Parameter = It.Value;

					const TSharedRef<FVoxelMaterialDefinitionParameterSchemaAction> NewParameterAction = MakeVoxelShared<FVoxelMaterialDefinitionParameterSchemaAction>(
						FText::FromString(Parameter.Category),
						FText::FromName(Parameter.Name),
						FText::FromString(Parameter.Description),
						1,
						FText::FromString(Parameter.Type.ToString()),
						1);

					NewParameterAction->Guid = It.Key;
					NewParameterAction->MaterialDefinition = MaterialDefinition;

					OutAllActions.AddAction(NewParameterAction);
				}
			})
			.OnCreateWidgetForAction_Lambda([=](FCreateWidgetForActionData* CreateData)
			{
				return
					SNew(SVoxelMaterialDefinitionParameterPaletteItem, CreateData)
					.Parameters(SharedThis(this));
			})
			.OnCanRenameSelectedAction_Lambda([](TWeakPtr<FGraphActionNode>)
			{
				return true;
			})
			.OnActionSelected_Lambda([this](const TArray<TSharedPtr<FEdGraphSchemaAction>>& Actions, const ESelectInfo::Type SelectionType)
			{
				const TSharedPtr<FVoxelMaterialDefinitionToolkit> Toolkit = GetToolkit();
				if (!ensure(Toolkit))
				{
					return;
				}

				if (Actions.Num() == 0)
				{
					if (!bIsRefreshing &&
						SelectionType != ESelectInfo::Direct)
					{
						Toolkit->SelectParameter({}, false, false);
					}
					return;
				}

				if (SelectionType != ESelectInfo::OnMouseClick &&
					SelectionType != ESelectInfo::OnKeyPress &&
					SelectionType != ESelectInfo::OnNavigation)
				{
					return;
				}

				if (!ensure(Actions.Num() == 1))
				{
					return;
				}

				const TSharedPtr<FEdGraphSchemaAction> Action = Actions[0];
				const FGuid Guid = static_cast<FVoxelMaterialDefinitionParameterSchemaAction&>(*Action).Guid;

				Toolkit->SelectParameter(Guid, false, false);
			})
			.OnActionMatchesName_Lambda([](FEdGraphSchemaAction* InAction, const FName& InName)
			{
				if (!ensure(InAction->GetTypeId() == FVoxelMaterialDefinitionParameterSchemaAction::StaticGetTypeId()))
				{
					return false;
				}

				return static_cast<FVoxelMaterialDefinitionParameterSchemaAction&>(*InAction).Guid.ToString() == InName.ToString();
			})
			.OnActionDragged_Lambda([this](const TArray<TSharedPtr<FEdGraphSchemaAction>>& Actions, const FPointerEvent& MouseEvent)
			{
				if (!ensure(Actions.Num() == 1))
				{
					return FReply::Unhandled();
				}

				const TSharedRef<FEdGraphSchemaAction> Action = Actions[0].ToSharedRef();

				return FReply::Handled().BeginDragDrop(FVoxelMaterialDefinitionParameterDragDropAction::New(
					SharedThis(this),
					StaticCastSharedRef<FVoxelMaterialDefinitionParameterSchemaAction>(Action)));
			})
			.OnCategoryTextCommitted_Lambda([this](const FText& InNewText, ETextCommit::Type, const TWeakPtr<FGraphActionNode> InAction)
			{
				const TSharedPtr<FVoxelMaterialDefinitionToolkit> Toolkit = GetToolkit();
				if (!ensure(Toolkit))
				{
					return;
				}
				UVoxelMaterialDefinition* MaterialDefinition = Toolkit->Asset;

				const TSharedPtr<FGraphActionNode> CategoryAction = InAction.Pin();
				if (!ensure(CategoryAction))
				{
					return;
				}

				const FString NewCategory = InNewText.ToString().TrimStartAndEnd();

				TArray<TSharedPtr<FEdGraphSchemaAction>> Actions;
				MembersMenu->GetCategorySubActions(InAction, Actions);

				if (Actions.Num() > 0)
				{
					const FVoxelTransaction Transaction(Toolkit->GetAsset(), "Rename Category");

					for (const TSharedPtr<FEdGraphSchemaAction>& Action : Actions)
					{
						const FGuid Guid = static_cast<FVoxelMaterialDefinitionParameterSchemaAction&>(*Action).Guid;

						FVoxelMaterialDefinitionParameter* Parameter = MaterialDefinition->GuidToMaterialParameter.Find(Guid);
						if (!ensure(Parameter))
						{
							continue;
						}

						Parameter->Category = NewCategory;
					}
				}

				bRefreshQueued = true;
				MembersMenu->SelectItemByName(FName(NewCategory), ESelectInfo::OnMouseClick, CategoryAction->SectionID, true);
			})
			.OnGetFilterText_Lambda([this]
			{
				return FilterBox->GetText();
			})
		]
	];
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelMaterialDefinitionParameters::SelectMember(const FGuid& Guid, const int32 SectionId, const bool bRequestRename, const bool bRefresh)
{
	if (bRefresh)
	{
		Refresh();
	}

	MembersMenu->SelectItemByName(*Guid.ToString(), ESelectInfo::Direct, SectionId);

	if (bRequestRename)
	{
		MembersMenu->OnRequestRenameOnActionNode();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelMaterialDefinitionParameters::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bRefreshQueued)
	{
		Refresh();
	}
}

FReply SVoxelMaterialDefinitionParameters::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (CommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelMaterialDefinitionParameters::PostUndo(bool bSuccess)
{
	Refresh();
}

void SVoxelMaterialDefinitionParameters::PostRedo(bool bSuccess)
{
	Refresh();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelMaterialDefinitionParameters::Refresh()
{
	bRefreshQueued = false;

	TGuardValue<bool> Guard(bIsRefreshing, true);
	MembersMenu->RefreshAllActions(true);
}

FString SVoxelMaterialDefinitionParameters::GetPasteCategory() const
{
	TArray<TSharedPtr<FEdGraphSchemaAction>> SelectedActions;
	MembersMenu->GetSelectedActions(SelectedActions);

	if (SelectedActions.Num() != 0 ||
		!MembersMenu.IsValid())
	{
		return "";
	}

	return MembersMenu->GetSelectedCategoryName();
}

void SVoxelMaterialDefinitionParameters::OnAddNewMember() const
{
	const TSharedPtr<FVoxelMaterialDefinitionToolkit> Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}
	UVoxelMaterialDefinition* MaterialDefinition = Toolkit->Asset;

	const FGuid Guid = FGuid::NewGuid();
	{
		const FVoxelTransaction Transaction(MaterialDefinition, "Create new parameter");

		FVoxelMaterialDefinitionParameter NewParameter;
		NewParameter.Name = "NewParameter";
		NewParameter.Category = GetPasteCategory();
		NewParameter.Type = FVoxelPinType::Make<float>();
		MaterialDefinition->GuidToMaterialParameter.Add(Guid, NewParameter);
	}

	Toolkit->SelectParameter(Guid, true, true);
}

TSharedPtr<SWidget> SVoxelMaterialDefinitionParameters::OnContextMenuOpening()
{
	FMenuBuilder MenuBuilder(true, CommandList);

	if (const TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> Action = GetSelectedAction())
	{
		MenuBuilder.BeginSection("BasicOperations");
		{
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Rename, NAME_None, INVTEXT("Rename"), INVTEXT("Renames this"));
			MenuBuilder.AddMenuEntry(FGraphEditorCommands::Get().FindReferences);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Cut);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Copy);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Duplicate);
			MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);
		}
		MenuBuilder.EndSection();
		return MenuBuilder.MakeWidget();
	}

	MenuBuilder.BeginSection("BasicOperations");
	{
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Paste);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("AddNewItem", INVTEXT("Add New"));
	{
		MenuBuilder.AddMenuEntry(
			INVTEXT("Add new Parameter"),
			FText(),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "BlueprintEditor.AddNewVariable"),
			FUIAction
			{
				FExecuteAction::CreateSP(this, &SVoxelMaterialDefinitionParameters::OnAddNewMember)
			});
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> SVoxelMaterialDefinitionParameters::GetSelectedAction() const
{
	TArray<TSharedPtr<FEdGraphSchemaAction>> SelectedActions;
	MembersMenu->GetSelectedActions(SelectedActions);

	if (SelectedActions.Num() == 0)
	{
		return nullptr;
	}

	const TSharedPtr<FEdGraphSchemaAction> Action = SelectedActions[0];
	if (!ensure(Action->GetTypeId() == FVoxelMaterialDefinitionParameterSchemaAction::StaticGetTypeId()))
	{
		return nullptr;
	}

	return StaticCastSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction>(Action);
}