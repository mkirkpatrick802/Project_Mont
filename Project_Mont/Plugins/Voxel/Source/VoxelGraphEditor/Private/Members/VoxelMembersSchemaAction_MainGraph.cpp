// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMembersSchemaAction_MainGraph.h"
#include "VoxelGraph.h"
#include "VoxelGraphToolkit.h"
#include "VoxelTerminalGraph.h"

UObject* FVoxelMembersSchemaAction_MainGraph::GetOuter() const
{
	return WeakGraph.Get();
}

TSharedRef<SWidget> FVoxelMembersSchemaAction_MainGraph::CreatePaletteWidget(const FCreateWidgetForActionData& CreateData) const
{
	return SNew(SVoxelMembersPaletteItem_MainGraph, CreateData);
}

void FVoxelMembersSchemaAction_MainGraph::BuildContextMenu(FMenuBuilder& MenuBuilder)
{
	if (!ensure(WeakGraph.IsValid()) ||
		!WeakGraph->GetBaseGraph_Unsafe())
	{
		return;
	}

	MenuBuilder.AddMenuEntry(
		INVTEXT("Go to parent"),
		INVTEXT("Go to the parent graph"),
		{},
		FUIAction
		{
			MakeLambdaDelegate([=]
			{
				const UVoxelGraph* Graph = WeakGraph.Get();
				if (!ensure(Graph))
				{
					return;
				}

				UVoxelGraph* BaseGraph = Graph->GetBaseGraph_Unsafe();
				if (!BaseGraph)
				{
					return;
				}

				FVoxelUtilities::FocusObject(BaseGraph->GetMainTerminalGraph());
			})
		});
}

void FVoxelMembersSchemaAction_MainGraph::ApplyNewGuids(const TArray<FGuid>& NewGuids) const
{
	ensure(NewGuids.Num() == 1);
}

void FVoxelMembersSchemaAction_MainGraph::OnActionDoubleClick() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}

	FVoxelUtilities::FocusObject(Toolkit->Asset->GetMainTerminalGraph());
}

void FVoxelMembersSchemaAction_MainGraph::OnSelected() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}

	Toolkit->GetSelection().SelectMainGraph();
}

FString FVoxelMembersSchemaAction_MainGraph::GetName() const
{
	return "Main Graph";
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelMembersPaletteItem_MainGraph::Construct(const FArguments& InArgs, const FCreateWidgetForActionData& CreateData)
{
	ActionPtr = CreateData.Action;

	ChildSlot
	[
		SNew(SBox)
		.Padding(FMargin(0.f, -2.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(FMargin(0.f, 2.f, 4.f, 2.f))
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				SNew(SImage)
				.Image(FVoxelEditorStyle::GetBrush("VoxelGraph.Execute"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(0.f)
			[
				CreateTextSlotWidget(CreateData)
			]
		]
	];
}