// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Members/SVoxelGraphMembers.h"
#include "Members/VoxelMembersSchemaAction_MainGraph.h"
#include "Members/VoxelMembersSchemaAction_Function.h"
#include "Members/VoxelMembersSchemaAction_Parameter.h"
#include "Members/VoxelMembersSchemaAction_Input.h"
#include "Members/VoxelMembersSchemaAction_Output.h"
#include "Members/VoxelMembersSchemaAction_LocalVariable.h"
#include "Members/VoxelMembersSchemaAction_LibraryFunction.h"
#include "Members/VoxelMembersDragDropAction_Category.h"
#include "VoxelGraphTracker.h"
#include "VoxelGraphToolkit.h"
#include "VoxelTerminalGraph.h"
#include "VoxelGraphSchemaAction.h"
#include "Nodes/VoxelGraphNode.h"
#include "Widgets/SVoxelGraphSearch.h"
#include "SGraphPanel.h"

#if VOXEL_ENGINE_VERSION >= 503
#include "GraphActionNode.h"
#else
#include "GraphEditor/Private/GraphActionNode.h"
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelGraphMembers::Construct(const FArguments& Args)
{
	WeakToolkit = Args._Toolkit;
	check(WeakToolkit.IsValid());

	const FOnVoxelGraphChanged OnChanged = FOnVoxelGraphChanged::Make(this, [this]
	{
		Refresh();
	});

	GVoxelGraphTracker->OnTerminalGraphChanged(*Args._Toolkit->Asset).Add(OnChanged);
	GVoxelGraphTracker->OnParameterChanged(*Args._Toolkit->Asset).Add(OnChanged);

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	CommandList = MakeVoxelShared<FUICommandList>();

	CommandList->MapAction(FGenericCommands::Get().Delete, MakeWeakPtrDelegate(this, [this]
	{
		const TSharedPtr<FVoxelMembersSchemaAction_Base> Action = GetSelectedAction();
		if (!Action)
		{
			return;
		}

		Action->OnDelete();
	}),
	MakeWeakPtrDelegate(this, [this]
	{
		const TSharedPtr<FVoxelMembersSchemaAction_Base> Action = GetSelectedAction();
		if (!Action)
		{
			return false;
		}

		return Action->CanBeDeleted();
	}));

	CommandList->MapAction(FGenericCommands::Get().Duplicate, MakeWeakPtrDelegate(this, [this]
	{
		const TSharedPtr<FVoxelMembersSchemaAction_Base> Action = GetSelectedAction();
		if (!Action)
		{
			return;
		}

		Action->OnDuplicate();
	}));

	CommandList->MapAction(FGenericCommands::Get().Rename, MakeWeakPtrDelegate(this, [this]
	{
		MembersMenu->OnRequestRenameOnActionNode();
	}),
	MakeWeakPtrDelegate(this, [this]
	{
		const TSharedPtr<FVoxelMembersSchemaAction_Base> Action = GetSelectedAction();
		if (!Action)
		{
			return false;
		}

		return
			Action->CanBeRenamed() &&
			MembersMenu->CanRequestRenameOnActionNode();
	}));

	CommandList->MapAction(FGenericCommands::Get().Copy, MakeWeakPtrDelegate(this, [this]
	{
		const TSharedPtr<FVoxelMembersSchemaAction_Base> Action = GetSelectedAction();
		if (!Action)
		{
			return;
		}

		const FString SectionName = LexToString(GetSection(Action->SectionID));
		if (!ensure(!SectionName.IsEmpty()))
		{
			return;
		}

		FString Text;
		if (!Action->OnCopy(Text))
		{
			return;
		}

		Text = "SVoxelGraphMembers." + SectionName + "." + Text;

		FPlatformApplicationMisc::ClipboardCopy(*Text);
	}));
	CommandList->MapAction(FGenericCommands::Get().Cut, MakeWeakPtrDelegate(this, [this]
	{
		const TSharedPtr<FVoxelMembersSchemaAction_Base> Action = GetSelectedAction();
		if (!Action)
		{
			return;
		}

		const FString SectionTitle = LexToString(GetSection(Action->SectionID));
		if (!ensure(!SectionTitle.IsEmpty()))
		{
			return;
		}

		FString Text;
		if (!Action->OnCopy(Text))
		{
			return;
		}

		Text = "SVoxelGraphMembers." + SectionTitle + "." + Text;
		FPlatformApplicationMisc::ClipboardCopy(*Text);

		Action->OnDelete();
	}),
	MakeWeakPtrDelegate(this, [this]
	{
		const TSharedPtr<FVoxelMembersSchemaAction_Base> Action = GetSelectedAction();
		if (!Action)
		{
			return false;
		}

		return Action->CanBeDeleted();
	}));

	CommandList->MapAction(FGenericCommands::Get().Paste, MakeWeakPtrDelegate(this, [this]
	{
		FString ClipboardText;
		FPlatformApplicationMisc::ClipboardPaste(ClipboardText);

		int32 SectionId = -1;
		if (!TryParsePastedText(ClipboardText, SectionId))
		{
			return;
		}

		const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
		if (!ensure(Toolkit))
		{
			return;
		}

		UVoxelTerminalGraph* TerminalGraph = GetTerminalGraph();
		if (!ensure(TerminalGraph))
		{
			return;
		}

		switch (GetSection(SectionId))
		{
		default:
		{
			ensure(false);
			return;
		}
		case EVoxelGraphMemberSection::Functions:
		{
			const UVoxelTerminalGraph* FunctionGraph = FindObject<UVoxelTerminalGraph>(nullptr, *ClipboardText);
			if (!FunctionGraph)
			{
				return;
			}

			FVoxelMembersSchemaAction_Function::OnPaste(*Toolkit, FunctionGraph);
		}
		break;
		case EVoxelGraphMemberSection::Parameters:
		{
			FVoxelMembersSchemaAction_Parameter::OnPaste(*Toolkit, ClipboardText, GetPasteCategory());
		}
		break;
		case EVoxelGraphMemberSection::GraphInputs:
		{
			FVoxelMembersSchemaAction_Input::OnPaste(*Toolkit, TerminalGraph->GetGraph().GetMainTerminalGraph(), ClipboardText, GetPasteCategory());
		}
		break;
		case EVoxelGraphMemberSection::FunctionInputs:
		{
			FVoxelMembersSchemaAction_Input::OnPaste(*Toolkit, *TerminalGraph, ClipboardText, GetPasteCategory());
		}
		break;
		case EVoxelGraphMemberSection::Outputs:
		{
			FVoxelMembersSchemaAction_Output::OnPaste(*Toolkit, *TerminalGraph, ClipboardText, GetPasteCategory());
		}
		break;
		case EVoxelGraphMemberSection::LocalVariables:
		{
			FVoxelMembersSchemaAction_LocalVariable::OnPaste(*Toolkit, *TerminalGraph, ClipboardText, GetPasteCategory());
		}
		break;
		}
	}),
	MakeWeakPtrDelegate(this, [this]
	{
		FString ClipboardText;
		FPlatformApplicationMisc::ClipboardPaste(ClipboardText);

		int32 SectionId = -1;
		return TryParsePastedText(ClipboardText, SectionId);
	}));

	CommandList->MapAction(FGraphEditorCommands::Get().FindReferences, MakeWeakPtrDelegate(this, [this]
	{
		const TSharedPtr<FVoxelMembersSchemaAction_Base> Action = GetSelectedAction();
		if (!Action)
		{
			return;
		}

		const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
		if (!ensure(Toolkit))
		{
			return;
		}

		Toolkit->GetSearchWidget()->FocusForUse(Action->GetSearchString());
	}));

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	SAssignNew(FilterBox, SSearchBox)
	.OnTextChanged_Lambda([this](const FText&)
	{
		MembersMenu->GenerateFilteredItems(false);
	});

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

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
			SAssignNew(MembersMenu, SVoxelGraphActionMenu, false)
			.AlphaSortItems(false)
			.UseSectionStyling(true)
			.AutoExpandActionMenu(true)
			.OnCollectStaticSections_Lambda([=](TArray<int32>& StaticSectionIds)
			{
				const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
				if (!ensure(Toolkit))
				{
					return;
				}

				if (Toolkit->Asset->IsFunctionLibrary())
				{
					StaticSectionIds.Add(GetSectionId(EVoxelGraphMemberSection::Functions));
				}
				else
				{
					StaticSectionIds.Add(GetSectionId(EVoxelGraphMemberSection::MainGraph));
					StaticSectionIds.Add(GetSectionId(EVoxelGraphMemberSection::Functions));
				}

				if (const UVoxelTerminalGraph* TerminalGraph = GetTerminalGraph())
				{
					if (TerminalGraph->IsMainTerminalGraph())
					{
						StaticSectionIds.Add(GetSectionId(EVoxelGraphMemberSection::GraphInputs));
						StaticSectionIds.Add(GetSectionId(EVoxelGraphMemberSection::Outputs));
					}
					else
					{
						StaticSectionIds.Add(GetSectionId(EVoxelGraphMemberSection::GraphInputs));
						StaticSectionIds.Add(GetSectionId(EVoxelGraphMemberSection::FunctionInputs));
						StaticSectionIds.Add(GetSectionId(EVoxelGraphMemberSection::Outputs));
					}
				}

				StaticSectionIds.Add(GetSectionId(EVoxelGraphMemberSection::Parameters));

				if (GetTerminalGraph())
				{
					StaticSectionIds.Add(GetSectionId(EVoxelGraphMemberSection::LocalVariables));
				}
			})
			.OnGetSectionTitle_Lambda([=](const int32 SectionId) -> FText
			{
				switch (GetSection(SectionId))
				{
				default: ensure(false); return {};
				case EVoxelGraphMemberSection::MainGraph: return INVTEXT("Graph");
				case EVoxelGraphMemberSection::Functions:
				{
					const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
					if (!ensure(Toolkit))
					{
						return {};
					}
					const UVoxelGraph& Graph = *Toolkit->Asset;

					int32 NumOverridable = 0;
					for (const FGuid& Guid : Graph.GetTerminalGraphs())
					{
						if (!Graph.FindTerminalGraph_NoInheritance(Guid))
						{
							NumOverridable++;
						}
					}

					if (NumOverridable == 0)
					{
						return INVTEXT("Functions");
					}

					return FText::Format(
						INVTEXT("Functions <TinyText.Subdued>({0} Overridable)</>"),
						FText::AsNumber(NumOverridable));
				}
				case EVoxelGraphMemberSection::Parameters: return INVTEXT("Parameters");
				case EVoxelGraphMemberSection::GraphInputs: return INVTEXT("Graph Inputs");
				case EVoxelGraphMemberSection::FunctionInputs: return INVTEXT("Function Inputs");
				case EVoxelGraphMemberSection::Outputs:
				{
					const UVoxelTerminalGraph* TerminalGraph = GetTerminalGraph();
					if (!ensure(TerminalGraph))
					{
						return {};
					}

					if (TerminalGraph->IsMainTerminalGraph())
					{
						return INVTEXT("Graph Outputs");
					}
					else
					{
						return INVTEXT("Function Outputs");
					}
				}
				case EVoxelGraphMemberSection::LocalVariables: return INVTEXT("Local Variables");
				}
			})
			.OnGetSectionWidget_Lambda([=](TSharedRef<SWidget> RowWidget, const int32 SectionId) -> TSharedRef<SWidget>
			{
				const auto CreateAddButton = [&](const FText& AddText)
				{
					return
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.OnClicked_Lambda([this, SectionId]
						{
							OnAddNewMember(SectionId);
							return FReply::Handled();
						})
						.ContentPadding(FMargin(1, 0))
						.ToolTipText(AddText)
						[
							SNew(SImage)
							.Image(FAppStyle::Get().GetBrush("Icons.PlusCircle"))
							.ColorAndOpacity(FSlateColor::UseForeground())
						];
				};

				switch (GetSection(SectionId))
				{
				default: ensure(false); return SNullWidget::NullWidget;
				case EVoxelGraphMemberSection::MainGraph: return SNullWidget::NullWidget;
				case EVoxelGraphMemberSection::Functions:
				{
					return SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SComboButton)
							.Visibility_Lambda([=]
							{
								const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
								if (!ensure(Toolkit))
								{
									return EVisibility::Collapsed;
								}
								const UVoxelGraph& Graph = *Toolkit->Asset;

								for (const FGuid& Guid : Graph.GetTerminalGraphs())
								{
									if (!Graph.FindTerminalGraph_NoInheritance(Guid))
									{
										return EVisibility::Visible;
									}
								}
								return EVisibility::Collapsed;
							})
							.ForegroundColor(FAppStyle::GetSlateColor("DefaultForeground"))
							.OnGetMenuContent_Lambda([=]
							{
								const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
								if (!ensure(Toolkit))
								{
									return SNullWidget::NullWidget;
								}
								UVoxelGraph& Graph = *Toolkit->Asset;

								FMenuBuilder MenuBuilder(true, CommandList);
								MenuBuilder.BeginSection("OverrideFunction", INVTEXT("Override Function"));
								{
									for (const FGuid& Guid : Graph.GetTerminalGraphs())
									{
										if (Graph.FindTerminalGraph_NoInheritance(Guid))
										{
											continue;
										}

										const UVoxelTerminalGraph* TerminalGraph = Graph.FindTopmostTerminalGraph(Guid);
										if (!ensure(TerminalGraph))
										{
											continue;
										}
										const FVoxelGraphMetadata Metadata = TerminalGraph->GetMetadata();

										MenuBuilder.AddMenuEntry(
											FUIAction(MakeLambdaDelegate([=, &Graph]
											{
												const FVoxelTransaction Transaction(Graph, "Override function");
												UVoxelTerminalGraph& NewTerminalGraph = Graph.AddTerminalGraph(Guid);
												SetupTerminalGraphOverride(NewTerminalGraph);
											})),
											SNew(SHorizontalBox)
											+ SHorizontalBox::Slot()
											.AutoWidth()
											.VAlign(VAlign_Center)
											.HAlign(HAlign_Left)
											.Padding(FMargin(2.0f, 0.0f, 20.0f, 0.0f))
											[
												SNew(STextBlock)
												.Text(FText::FromString(Metadata.DisplayName))
												.ToolTipText(FText::FromString(Metadata.Description))
											]
											+ SHorizontalBox::Slot()
											.VAlign(VAlign_Center)
											.HAlign(HAlign_Right)
											.Padding(FMargin(1.0f, 0.0f, 0.0f, 0.0f))
											[
												SNew(STextBlock)
												.Text(FText::FromString(TerminalGraph->GetGraph().GetName()))
												.ToolTipText(FText::FromString(TerminalGraph->GetGraph().GetPathName()))
												.ColorAndOpacity(FSlateColor::UseSubduedForeground())
											]);
									}
								}
								return MenuBuilder.MakeWidget();
							})
							.ContentPadding(0.0f)
							.HasDownArrow(true)
							.ButtonContent()
							[
								SNew(STextBlock)
								.Font(IDetailLayoutBuilder::GetDetailFontBold())
								.Text(INVTEXT("Override"))
							]
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(2,0,0,0)
						[
							CreateAddButton(INVTEXT("Add new function"))
						];
				}
				case EVoxelGraphMemberSection::Parameters: return CreateAddButton(INVTEXT("Add new parameter"));
				case EVoxelGraphMemberSection::GraphInputs: return CreateAddButton(INVTEXT("Add new graph input"));
				case EVoxelGraphMemberSection::FunctionInputs: return CreateAddButton(INVTEXT("Add new function input"));
				case EVoxelGraphMemberSection::Outputs: return CreateAddButton(INVTEXT("Add new output"));
				case EVoxelGraphMemberSection::LocalVariables: return CreateAddButton(INVTEXT("Add new local variable"));
				}
			})
			.OnContextMenuOpening_Lambda([=]
			{
				FMenuBuilder MenuBuilder(true, CommandList);

				if (const TSharedPtr<FVoxelMembersSchemaAction_Base> Action = GetSelectedAction())
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

					MenuBuilder.BeginSection("CustomOperations");
					Action->BuildContextMenu(MenuBuilder);
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
						INVTEXT("Add new Function"),
						FText(),
						FSlateIcon(FAppStyle::GetAppStyleSetName(), "BlueprintEditor.AddNewFunction"),
						FUIAction{
							FExecuteAction::CreateSP(this, &SVoxelGraphMembers::OnAddNewMember, GetSectionId(EVoxelGraphMemberSection::Functions))
						});

					if (const UVoxelTerminalGraph* TerminalGraph = GetTerminalGraph())
					{
						MenuBuilder.AddMenuEntry(
							INVTEXT("Add new graph input"),
							FText(),
							FSlateIcon(FAppStyle::GetAppStyleSetName(), "BlueprintEditor.AddNewVariable"),
							FUIAction
							{
								MakeWeakPtrDelegate(this, [=]
								{
									OnAddNewMember(GetSectionId(EVoxelGraphMemberSection::GraphInputs));
								})
							});

						if (!TerminalGraph->IsMainTerminalGraph())
						{
							MenuBuilder.AddMenuEntry(
								INVTEXT("Add new function input"),
								FText(),
								FSlateIcon(FAppStyle::GetAppStyleSetName(), "BlueprintEditor.AddNewVariable"),
								FUIAction
								{
									MakeWeakPtrDelegate(this, [=]
									{
										OnAddNewMember(GetSectionId(EVoxelGraphMemberSection::FunctionInputs));
									})
								});
						}

						MenuBuilder.AddMenuEntry(
							INVTEXT("Add new output"),
							FText(),
							FSlateIcon(FAppStyle::GetAppStyleSetName(), "BlueprintEditor.AddNewVariable"),
							FUIAction
							{
								MakeWeakPtrDelegate(this, [=]
								{
									OnAddNewMember(GetSectionId(EVoxelGraphMemberSection::Outputs));
								})
							});

						MenuBuilder.AddMenuEntry(
							INVTEXT("Add new parameter"),
							FText(),
							FSlateIcon(FAppStyle::GetAppStyleSetName(), "BlueprintEditor.AddNewVariable"),
							FUIAction
							{
								MakeWeakPtrDelegate(this, [=]
								{
									OnAddNewMember(GetSectionId(EVoxelGraphMemberSection::Parameters));
								})
							});

						MenuBuilder.AddMenuEntry(
							INVTEXT("Add new local variable"),
							FText(),
							FSlateIcon(FAppStyle::GetAppStyleSetName(), "BlueprintEditor.AddNewLocalVariable"),
							FUIAction
							{
								MakeWeakPtrDelegate(this, [=]
								{
									OnAddNewMember(GetSectionId(EVoxelGraphMemberSection::LocalVariables));
								})
							});
					}
				}
				MenuBuilder.EndSection();

				return MenuBuilder.MakeWidget();
			})
			.OnCollectAllActions_Lambda([=](FGraphActionListBuilderBase& OutAllActions)
			{
				TMap<int32, TMap<FString, TArray<TSharedRef<FEdGraphSchemaAction>>>> SectionIdToCategoryToActions;
				for (const TSharedRef<FVoxelMembersSchemaAction_Base>& Action : GetActions())
				{
					SectionIdToCategoryToActions.FindOrAdd(Action->SectionID).FindOrAdd(Action->GetCategory()).Add(Action);
				}

				for (const auto& SectionIdIt : SectionIdToCategoryToActions)
				{
					for (const auto& CategoryIt : SectionIdIt.Value)
					{
						for (const TSharedRef<FEdGraphSchemaAction>& Action : CategoryIt.Value)
						{
							OutAllActions.AddAction(Action);
						}
					}
				}
			})
			.OnCreateWidgetForAction_Lambda([](const FCreateWidgetForActionData* CreateData)
			{
				return static_cast<FVoxelMembersSchemaAction_Base&>(*CreateData->Action).CreatePaletteWidget(*CreateData);
			})
			.OnCanRenameSelectedAction_Lambda([](const TWeakPtr<FGraphActionNode> WeakActionNode)
			{
				const TSharedPtr<FGraphActionNode> ActionNode = WeakActionNode.Pin();
				if (!ensure(ActionNode))
				{
					return false;
				}

				for (const TSharedPtr<FEdGraphSchemaAction> Action : ActionNode->Actions)
				{
					if (!Action->CanBeRenamed())
					{
						return false;
					}
				}
				return true;
			})
			.OnActionSelected_Lambda([this](const TArray<TSharedPtr<FEdGraphSchemaAction>>& Actions, const ESelectInfo::Type InSelectionType)
			{
				const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
				if (!ensure(Toolkit))
				{
					return;
				}

				if (Actions.Num() == 0)
				{
					if (!bIsRefreshing &&
						InSelectionType != ESelectInfo::Direct)
					{
						const TArray<TSharedPtr<FGraphActionNode>> SelectedNodes = MembersMenu->GetSelectedNodes();
						if (SelectedNodes.Num() == 1)
						{
							const FGraphActionNode& SelectedNode = *SelectedNodes[0];
							Toolkit->GetSelection().SelectCategory(
								SelectedNode.SectionID,
								FName(SelectedNode.GetDisplayName().ToString()));
						}
						else
						{
							Toolkit->GetSelection().SelectNone();
						}
					}
					return;
				}

				if (InSelectionType != ESelectInfo::OnMouseClick &&
					InSelectionType != ESelectInfo::OnKeyPress &&
					InSelectionType != ESelectInfo::OnNavigation)
				{
					return;
				}

				if (!ensure(Actions.Num() == 1))
				{
					return;
				}

				const TSharedPtr<FEdGraphSchemaAction> Action = Actions[0];
				static_cast<FVoxelMembersSchemaAction_Base&>(*Action).OnSelected();
			})
			.OnActionMatchesName_Lambda([](FEdGraphSchemaAction* InAction, const FName& InName)
			{
				if (!ensure(InAction->GetTypeId() == FVoxelMembersSchemaAction_Base::StaticGetTypeId()))
				{
					return false;
				}

				const FVoxelMembersSchemaAction_Base& Action = static_cast<FVoxelMembersSchemaAction_Base&>(*InAction);
				if (Action.IsDeleted())
				{
					return false;
				}

				return
					Action.MemberGuid.ToString() == InName.ToString() ||
					Action.GetName() == InName.ToString();
			})
			.OnActionDragged_Lambda([this](const TArray<TSharedPtr<FEdGraphSchemaAction>>& Actions, const FPointerEvent& MouseEvent)
			{
				if (!ensure(Actions.Num() == 1))
				{
					return FReply::Unhandled();
				}

				const TSharedPtr<FEdGraphSchemaAction> Action = Actions[0];
				return static_cast<FVoxelMembersSchemaAction_Base&>(*Action).OnDragged(MouseEvent);
			})
			.OnCategoryTextCommitted_Lambda([this](const FText& InNewText, ETextCommit::Type, const TWeakPtr<FGraphActionNode> InAction)
			{
				const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
				if (!ensure(Toolkit))
				{
					return;
				}

				const TSharedPtr<FGraphActionNode> CategoryAction = InAction.Pin();
				if (!ensure(CategoryAction))
				{
					return;
				}

				const FString CategoryName = InNewText.ToString().TrimStartAndEnd();

				TArray<TSharedPtr<FEdGraphSchemaAction>> Actions;
				MembersMenu->GetCategorySubActions(InAction, Actions);

				if (Actions.Num() > 0)
				{
					const FVoxelTransaction Transaction(Toolkit->GetAsset(), "Rename Category");

					for (const TSharedPtr<FEdGraphSchemaAction>& Action : Actions)
					{
						static_cast<FVoxelMembersSchemaAction_Base&>(*Action).SetCategory(CategoryName);
					}
				}

				Refresh();
				MembersMenu->SelectItemByName(FName(*CategoryName), ESelectInfo::OnMouseClick, CategoryAction->SectionID, true);
			})
			.OnCategoryDragged_Lambda([this](const FText& InCategory, const FPointerEvent& MouseEvent)
			{
				TArray<TSharedPtr<FEdGraphSchemaAction>> Actions;
				MembersMenu->GetSelectedCategorySubActions(Actions);

				if (Actions.Num() == 0)
				{
					return FReply::Unhandled();
				}

				return FReply::Handled().BeginDragDrop(FVoxelMembersDragDropAction_Category::New(
					InCategory.ToString(),
					SharedThis(this),
					Actions[0]->SectionID));
			})
			.OnGetFilterText_Lambda([this]
			{
				return FilterBox->GetText();
			})
			.OnActionDoubleClicked_Lambda([](const TArray<TSharedPtr<FEdGraphSchemaAction>>& Actions)
			{
				if (!ensure(Actions.Num() == 1))
				{
					return;
				}

				const TSharedPtr<FEdGraphSchemaAction> Action = Actions[0];
				return static_cast<FVoxelMembersSchemaAction_Base&>(*Action).OnActionDoubleClick();
			})
		]
	];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FReply SVoxelGraphMembers::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (CommandList.IsValid() &&
		CommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelGraphMembers::Refresh()
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}

	// Force a flush to avoid double refresh putting us out of Rename
	GVoxelGraphTracker->Flush();

	ensureVoxelSlow(!bIsRefreshing);

	{
		TGuardValue<bool> Guard(bIsRefreshing, true);
		MembersMenu->RefreshAllActions(true);
	}

	Toolkit->GetSelection().Update();
}

void SVoxelGraphMembers::RequestRename()
{
	MembersMenu->OnRequestRenameOnActionNode();
}

void SVoxelGraphMembers::SetTerminalGraph(const TWeakObjectPtr<UVoxelTerminalGraph>& NewTerminalGraph)
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}

	OnPropertyChangedPtr = MakeSharedVoid();
	WeakTerminalGraph = NewTerminalGraph;

	if (!NewTerminalGraph.IsValid())
	{
		return;
	}

	const FOnVoxelGraphChanged OnPropertyChanged = FOnVoxelGraphChanged::Make(OnPropertyChangedPtr, this, [this]
	{
		Refresh();
	});

	GVoxelGraphTracker->OnInputChanged(*NewTerminalGraph).Add(OnPropertyChanged);
	GVoxelGraphTracker->OnOutputChanged(*NewTerminalGraph).Add(OnPropertyChanged);
	GVoxelGraphTracker->OnLocalVariableChanged(*NewTerminalGraph).Add(OnPropertyChanged);

	// For graph inputs
	GVoxelGraphTracker->OnInputChanged(NewTerminalGraph->GetGraph().GetMainTerminalGraph()).Add(OnPropertyChanged);

	Refresh();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelGraphMembers::SelectNone() const
{
	MembersMenu->SelectItemByName({});
}

void SVoxelGraphMembers::SelectCategory(
	const int32 SectionId,
	const FName DisplayName) const
{
	ensureVoxelSlow(MembersMenu->SelectItemByName(
		DisplayName,
		ESelectInfo::Direct,
		SectionId,
		true));
}

void SVoxelGraphMembers::SelectMember(
	const EVoxelGraphMemberSection Section,
	const FGuid& Guid) const
{
	ensureVoxelSlow(MembersMenu->SelectItemByName(
		FName(Guid.ToString()),
		ESelectInfo::Direct,
		GetSectionId(Section)));
}

void SVoxelGraphMembers::SelectClosestAction(const int32 SectionId, const FGuid& Guid) const
{
	const TSharedPtr Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}

	TSharedPtr<FVoxelMembersSchemaAction_Base> TargetAction;
	bool bFoundAction = false;
	for (const TSharedPtr<FGraphActionNode>& Node : MembersMenu->GetVisibleNodes())
	{
		if (Node->SectionID != SectionId ||
			!Node->IsActionNode() ||
			!Node->GetPrimaryAction())
		{
			continue;
		}

		const TSharedPtr<FVoxelMembersSchemaAction_Base> Action = StaticCastSharedPtr<FVoxelMembersSchemaAction_Base>(Node->GetPrimaryAction());
		if (Action->MemberGuid == Guid)
		{
			bFoundAction = true;
			continue;
		}

		TargetAction = Action;
		if (bFoundAction)
		{
			break;
		}
	}

	if (!TargetAction)
	{
		Toolkit->GetSelection().SelectNone();
		return;
	}

	TargetAction->OnSelected();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TArray<TSharedRef<FVoxelMembersSchemaAction_Base>> SVoxelGraphMembers::GetActions()
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return {};
	}
	UVoxelGraph& Graph = *Toolkit->Asset;

	TArray<TSharedRef<FVoxelMembersSchemaAction_Base>> OutActions;

	INLINE_LAMBDA
	{
		if (Graph.IsFunctionLibrary())
		{
			// Use a special action for libraries to handle visibility
			for (const FGuid& Guid : Graph.GetTerminalGraphs())
			{
				UVoxelTerminalGraph& TerminalGraph = Graph.FindTerminalGraphChecked(Guid);
				if (TerminalGraph.IsMainTerminalGraph())
				{
					continue;
				}
				const FVoxelGraphMetadata Metadata = TerminalGraph.GetMetadata();

				FString Description = Metadata.Description;
				if (!Description.IsEmpty())
				{
					Description += "\n\n";
				}
				Description += "GUID: " + Guid.ToString(EGuidFormats::DigitsWithHyphens);

				const TSharedRef<FVoxelMembersSchemaAction_LibraryFunction> Action = MakeVoxelShared<FVoxelMembersSchemaAction_LibraryFunction>(
					FText::FromString(Metadata.Category),
					FText::FromString(Metadata.DisplayName),
					FText::FromString(Description),
					0,
					INVTEXT("function"),
					GetSectionId(EVoxelGraphMemberSection::Functions));

				Action->MemberGuid = Guid;
				Action->WeakToolkit = Toolkit;
				Action->WeakMembers = SharedThis(this);
				Action->WeakTerminalGraph = &TerminalGraph;

				OutActions.Add(Action);
			}

			return;
		}

		{
			const TSharedRef<FVoxelMembersSchemaAction_MainGraph> Action = MakeVoxelShared<FVoxelMembersSchemaAction_MainGraph>(
				INVTEXT(""),
				INVTEXT("Main Graph"),
				INVTEXT(""),
				2,
				INVTEXT(""),
				GetSectionId(EVoxelGraphMemberSection::MainGraph));

			Action->MemberGuid = GVoxelMainTerminalGraphGuid;
			Action->WeakToolkit = Toolkit;
			Action->WeakMembers = SharedThis(this);
			Action->WeakGraph = &Graph;

			OutActions.Add(Action);
		}

		for (const FGuid& Guid : Graph.GetTerminalGraphs())
		{
			UVoxelTerminalGraph* TerminalGraph = Graph.FindTerminalGraph_NoInheritance(Guid);
			if (!TerminalGraph ||
				TerminalGraph->IsMainTerminalGraph())
			{
				continue;
			}
			const FVoxelGraphMetadata Metadata = TerminalGraph->GetMetadata();

			FString Description = Metadata.Description;
			if (!Description.IsEmpty())
			{
				Description += "\n\n";
			}
			Description += "GUID: " + Guid.ToString(EGuidFormats::DigitsWithHyphens);

			const TSharedRef<FVoxelMembersSchemaAction_Function> Action = MakeVoxelShared<FVoxelMembersSchemaAction_Function>(
				FText::FromString(Metadata.Category),
				FText::FromString(Metadata.DisplayName),
				FText::FromString(Description),
				2,
				INVTEXT("function"),
				GetSectionId(EVoxelGraphMemberSection::Functions));

			Action->MemberGuid = Guid;
			Action->WeakToolkit = Toolkit;
			Action->WeakMembers = SharedThis(this);
			Action->WeakTerminalGraph = TerminalGraph;

			OutActions.Add(Action);
		}
	};

	for (const FGuid& Guid : Graph.GetParameters())
	{
		const FVoxelParameter& Parameter = Graph.FindParameterChecked(Guid);

		const TSharedRef<FVoxelMembersSchemaAction_Parameter> Action = MakeVoxelShared<FVoxelMembersSchemaAction_Parameter>(
			FText::FromString(Parameter.Category),
			FText::FromName(Parameter.Name),
			FText::FromString(Parameter.Description),
			1,
			FText::FromString(Parameter.Type.ToString()),
			GetSectionId(EVoxelGraphMemberSection::Parameters));

		Action->bIsInherited = Graph.IsInheritedParameter(Guid);
		Action->MemberGuid = Guid;
		Action->WeakToolkit = Toolkit;
		Action->WeakMembers = SharedThis(this);
		Action->WeakGraph = &Graph;

		OutActions.Add(Action);
	}

	UVoxelTerminalGraph* TerminalGraph = GetTerminalGraph();
	if (!TerminalGraph)
	{
		return OutActions;
	}

	{
		UVoxelTerminalGraph& MainTerminalGraph = TerminalGraph->GetGraph().GetMainTerminalGraph();

		for (const FGuid& Guid : MainTerminalGraph.GetInputs())
		{
			const FVoxelGraphInput& Input = MainTerminalGraph.FindInputChecked(Guid);

			const TSharedRef<FVoxelMembersSchemaAction_Input> Action = MakeVoxelShared<FVoxelMembersSchemaAction_Input>(
				FText::FromString(Input.Category),
				FText::FromName(Input.Name),
				FText::FromString(Input.Description),
				1,
				FText::FromString(Input.Type.ToString()),
				GetSectionId(EVoxelGraphMemberSection::GraphInputs));

			Action->bIsInherited = MainTerminalGraph.IsInheritedInput(Guid);
			Action->MemberGuid = Guid;
			Action->WeakToolkit = Toolkit;
			Action->WeakMembers = SharedThis(this);
			Action->WeakTerminalGraph = &MainTerminalGraph;

			OutActions.Add(Action);
		}
	}

	if (!TerminalGraph->IsMainTerminalGraph())
	{
		for (const FGuid& Guid : TerminalGraph->GetInputs())
		{
			const FVoxelGraphInput& Input = TerminalGraph->FindInputChecked(Guid);

			const TSharedRef<FVoxelMembersSchemaAction_Input> Action = MakeVoxelShared<FVoxelMembersSchemaAction_Input>(
				FText::FromString(Input.Category),
				FText::FromName(Input.Name),
				FText::FromString(Input.Description),
				1,
				FText::FromString(Input.Type.ToString()),
				GetSectionId(EVoxelGraphMemberSection::FunctionInputs));

			Action->bIsInherited = TerminalGraph->IsInheritedInput(Guid);
			Action->MemberGuid = Guid;
			Action->WeakToolkit = Toolkit;
			Action->WeakMembers = SharedThis(this);
			Action->WeakTerminalGraph = TerminalGraph;

			OutActions.Add(Action);
		}
	}

	for (const FGuid& Guid : TerminalGraph->GetOutputs())
	{
		const FVoxelGraphOutput& Output = TerminalGraph->FindOutputChecked(Guid);

		const TSharedRef<FVoxelMembersSchemaAction_Output> Action = MakeVoxelShared<FVoxelMembersSchemaAction_Output>(
			FText::FromString(Output.Category),
			FText::FromName(Output.Name),
			FText::FromString(Output.Description),
			1,
			FText::FromString(Output.Type.ToString()),
			GetSectionId(EVoxelGraphMemberSection::Outputs));

		Action->bIsInherited = TerminalGraph->IsInheritedOutput(Guid);
		Action->MemberGuid = Guid;
		Action->WeakToolkit = Toolkit;
		Action->WeakMembers = SharedThis(this);
		Action->WeakTerminalGraph = TerminalGraph;

		OutActions.Add(Action);
	}

	for (const FGuid& Guid : TerminalGraph->GetLocalVariables())
	{
		const FVoxelGraphLocalVariable& LocalVariable = TerminalGraph->FindLocalVariableChecked(Guid);

		const TSharedRef<FVoxelMembersSchemaAction_LocalVariable> Action = MakeVoxelShared<FVoxelMembersSchemaAction_LocalVariable>(
			FText::FromString(LocalVariable.Category),
			FText::FromName(LocalVariable.Name),
			FText::FromString(LocalVariable.Description),
			1,
			FText::FromString(LocalVariable.Type.ToString()),
			GetSectionId(EVoxelGraphMemberSection::LocalVariables));

		Action->MemberGuid = Guid;
		Action->WeakToolkit = Toolkit;
		Action->WeakMembers = SharedThis(this);
		Action->WeakTerminalGraph = TerminalGraph;

		OutActions.Add(Action);
	}

	return OutActions;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelGraphMembers::PostUndo(const bool bSuccess)
{
	const TSharedPtr Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}

	if (!GetTerminalGraph())
	{
		Toolkit->GetSelection().SelectNone();
	}

	Refresh();
}

void SVoxelGraphMembers::PostRedo(const bool bSuccess)
{
	PostUndo(bSuccess);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString SVoxelGraphMembers::GetPasteCategory() const
{
	TArray<TSharedPtr<FEdGraphSchemaAction>> SelectedActions;
	MembersMenu->GetSelectedActions(SelectedActions);

	if (SelectedActions.Num() == 1)
	{
		return SelectedActions[0]->GetCategory().ToString();
	}
	if (SelectedActions.Num() > 1)
	{
		return {};
	}

	return MembersMenu->GetSelectedCategoryName();
}

void SVoxelGraphMembers::OnAddNewMember(const int32 SectionId)
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}

	UEdGraph* EdGraph = Toolkit->GetActiveEdGraph();
	const TSharedPtr<SGraphEditor> GraphEditor = Toolkit->GetActiveGraphEditor();
	if (!ensure(EdGraph) ||
		!ensure(GraphEditor))
	{
		return;
	}

	const FVector2D Location = INLINE_LAMBDA
	{
		const FGeometry CachedGeometry = GraphEditor->GetCachedGeometry();
		const FVector2D CenterLocation = GraphEditor->GetGraphPanel()->PanelCoordToGraphCoord(CachedGeometry.GetLocalSize() / 2.f);

		FVector2D Result = CenterLocation;

		const FVector2D StaticSize = FVector2D(100.f, 50.f);
		FBox2D CurrentBox(Result, Result + StaticSize);

		const FBox2D ViewportBox(
			GraphEditor->GetGraphPanel()->PanelCoordToGraphCoord(FVector2D::ZeroVector),
			GraphEditor->GetGraphPanel()->PanelCoordToGraphCoord(CachedGeometry.GetLocalSize()));

		int32 Iterations = 0;
		bool bFoundLocation = false;
		while (!bFoundLocation)
		{
			bFoundLocation = true;

			for (int32 Index = 0; Index < EdGraph->Nodes.Num(); Index++)
			{
				const UEdGraphNode* CurrentNode = EdGraph->Nodes[Index];
				const FVector2D NodePosition(CurrentNode->NodePosX, CurrentNode->NodePosY);
				FBox2D NodeBox(NodePosition, NodePosition);

				if (const TSharedPtr<SGraphNode> Widget = GraphEditor->GetGraphPanel()->GetNodeWidgetFromGuid(CurrentNode->NodeGuid))
				{
					if (Widget->GetCachedGeometry().GetLocalSize() == FVector2D::ZeroVector)
					{
						continue;
					}

					NodeBox.Max += Widget->GetCachedGeometry().GetAbsoluteSize() / GraphEditor->GetGraphPanel()->GetZoomAmount();
				}
				else
				{
					NodeBox.Max += StaticSize;
				}

				if (CurrentBox.Intersect(NodeBox))
				{
					Result.Y = NodeBox.Max.Y + 30.f;

					CurrentBox.Min = Result;
					CurrentBox.Max = Result + StaticSize;

					bFoundLocation = false;
					break;
				}
			}

			if (!CurrentBox.Intersect(ViewportBox))
			{
				Iterations++;
				if (Iterations == 1)
				{
					Result = CenterLocation + FVector2D(200.f, 0.f);
				}
				else if (Iterations == 2)
				{
					Result = CenterLocation - FVector2D(200.f, 0.f);
				}
				else
				{
					Result = CenterLocation;
					break;
				}

				CurrentBox.Min = Result;
				CurrentBox.Max = Result + StaticSize;

				bFoundLocation = false;
			}
		}

		return Result;
	};

	switch (GetSection(SectionId))
	{
	default:
	{
		ensure(false);
		return;
	}
	case EVoxelGraphMemberSection::Functions:
	{
		FVoxelGraphSchemaAction_NewFunction Action;
		Action.Category = GetPasteCategory();
		Action.PerformAction(EdGraph, nullptr, Location);
	}
	break;
	case EVoxelGraphMemberSection::Parameters:
	{
		FVoxelGraphSchemaAction_NewParameter Action;
		Action.Category = GetPasteCategory();
		Action.PerformAction(EdGraph, nullptr, Location);
	}
	break;
	case EVoxelGraphMemberSection::GraphInputs:
	{
		FVoxelGraphSchemaAction_NewGraphInput Action;
		Action.Category = GetPasteCategory();
		Action.PerformAction(EdGraph, nullptr, Location);
	}
	break;
	case EVoxelGraphMemberSection::FunctionInputs:
	{
		FVoxelGraphSchemaAction_NewFunctionInput Action;
		Action.Category = GetPasteCategory();
		Action.PerformAction(EdGraph, nullptr, Location);
	}
	break;
	case EVoxelGraphMemberSection::Outputs:
	{
		FVoxelGraphSchemaAction_NewOutput Action;
		Action.Category = GetPasteCategory();
		Action.PerformAction(EdGraph, nullptr, Location);
	}
	break;
	case EVoxelGraphMemberSection::LocalVariables:
	{
		FVoxelGraphSchemaAction_NewLocalVariable Action;
		Action.Category = GetPasteCategory();
		Action.PerformAction(EdGraph, nullptr, Location);
	}
	break;
	}
}

TSharedPtr<FVoxelMembersSchemaAction_Base> SVoxelGraphMembers::GetSelectedAction() const
{
	TArray<TSharedPtr<FEdGraphSchemaAction>> SelectedActions;
	MembersMenu->GetSelectedActions(SelectedActions);

	if (SelectedActions.Num() == 0)
	{
		return nullptr;
	}

	const TSharedPtr<FEdGraphSchemaAction> Action = SelectedActions[0];
	if (!ensure(Action->GetTypeId() == FVoxelMembersSchemaAction_Base::StaticGetTypeId()))
	{
		return nullptr;
	}

	return StaticCastSharedPtr<FVoxelMembersSchemaAction_Base>(Action);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool SVoxelGraphMembers::TryParsePastedText(FString& InOutText, int32& OutSectionId)
{
	if (!InOutText.RemoveFromStart("SVoxelGraphMembers."))
	{
		return false;
	}

	for (const EVoxelGraphMemberSection Section : TEnumRange<EVoxelGraphMemberSection>())
	{
		if (!InOutText.RemoveFromStart(LexToString(Section), ESearchCase::CaseSensitive))
		{
			continue;
		}

		if (!ensure(InOutText.RemoveFromStart(".")))
		{
			return false;
		}

		OutSectionId = GetSectionId(Section);
		return true;
	}

	return false;
}

void SVoxelGraphMembers::SetupTerminalGraphOverride(UVoxelTerminalGraph& TerminalGraph)
{
	UEdGraph& EdGraph = TerminalGraph.GetEdGraph();

	UVoxelGraphNode* CallParentNode = INLINE_LAMBDA
	{
		FVoxelGraphSchemaAction_NewCallFunctionNode Action;
		Action.Guid = TerminalGraph.GetGuid();
		Action.bCallParent = true;
		return Action.Apply(EdGraph, FVector2D::ZeroVector);
	};

	if (!ensure(CallParentNode))
	{
		return;
	}

	int32 InputIndex = 0;
	for (const FGuid& Guid : TerminalGraph.GetInputs())
	{
		FVoxelGraphSchemaAction_NewFunctionInputUsage Action;
		Action.Guid = Guid;

		const UVoxelGraphNode* InputNode = Action.Apply(EdGraph, FVector2D(-200.f, InputIndex * 75.f));
		if (!ensure(InputNode))
		{
			continue;
		}

		UEdGraphPin* OtherPin = CallParentNode->FindPin(Guid.ToString());
		if (!ensure(OtherPin))
		{
			continue;
		}

		InputNode->GetOutputPin(0)->MakeLinkTo(OtherPin);

		InputIndex++;
	}

	int32 OutputIndex = 0;
	for (const FGuid& Guid : TerminalGraph.GetOutputs())
	{
		FVoxelGraphSchemaAction_NewOutputUsage Action;
		Action.Guid = Guid;

		const UVoxelGraphNode* OutputNode = Action.Apply(EdGraph, FVector2D(300.f, OutputIndex * 75.f));
		if (!ensure(OutputNode))
		{
			continue;
		}

		UEdGraphPin* OtherPin = CallParentNode->FindPin(Guid.ToString());
		if (!ensure(OtherPin))
		{
			continue;
		}

		OutputNode->GetInputPin(0)->MakeLinkTo(OtherPin);

		OutputIndex++;
	}

	FVoxelUtilities::FocusObject(TerminalGraph);
}