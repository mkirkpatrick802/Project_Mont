// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Customizations/VoxelParameterChildBuilder.h"
#include "Customizations/VoxelParameterDetails.h"
#include "VoxelParameterView.h"
#include "VoxelCategoryBuilder.h"
#include "VoxelParameterOverridesOwner.h"
#include "VoxelParameterOverridesDetails.h"

#define private public
#include "Editor/PropertyEditor/Private/DetailItemNode.h"
#undef private

void FVoxelParameterChildBuilder::UpdateExpandedState()
{
	const TSharedPtr<const FDetailItemNode> Node = WeakNode.Pin();
	if (!Node)
	{
		return;
	}

#if INTELLISENSE_PARSER
	const bool bNewIsExpanded = this != nullptr;
#else
	const bool bNewIsExpanded = Node->bIsExpanded;
#endif
	if (bIsExpanded == bNewIsExpanded)
	{
		return;
	}

	bIsExpanded = bNewIsExpanded;

	if (!bIsExpanded)
	{
		VOXEL_SCOPE_COUNTER("Fixup DetailCustomWidgetExpansion");

		// Logic around DetailCustomWidgetExpansion is a bit funky,
		// and sometimes expanded items can be leaked to other actors/objects, making them always expanded.
		// To mitigate this, we force clear DetailCustomWidgetExpansion of anything containing our Path on collapse

		const FString Name = ParameterDetails.Path.ToString();
		TVoxelArray<FString> KeysToFix;

		GConfig->ForEachEntry(
			MakeLambdaDelegate([&](const TCHAR* Key, const TCHAR* Value)
			{
				if (FStringView(Value).Contains(Name))
				{
					KeysToFix.Add(Key);
				}
			}),
			TEXT("DetailCustomWidgetExpansion"),
			GEditorPerProjectIni);

		for (const FString& Key : KeysToFix)
		{
			FString ExpandedCustomItems;
			GConfig->GetString(TEXT("DetailCustomWidgetExpansion"), *Key, ExpandedCustomItems, GEditorPerProjectIni);

			TArray<FString> ExpandedCustomItemsArray;
			ExpandedCustomItems.ParseIntoArray(ExpandedCustomItemsArray, TEXT(","), true);

			ensure(ExpandedCustomItemsArray.RemoveAllSwap([&](const FString& Item)
			{
				return Item.Contains(Name);
			}));

			TStringBuilder<256> ExpandedCustomItemsBuilder;
			ExpandedCustomItemsBuilder.Join(ExpandedCustomItemsArray, TEXT(","));
			GConfig->SetString(TEXT("DetailCustomWidgetExpansion"), *Key, *ExpandedCustomItemsBuilder, GEditorPerProjectIni);
		}
	}

	(void)OnRegenerateChildren.ExecuteIfBound();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelParameterChildBuilder::SetOnRebuildChildren(const FSimpleDelegate InOnRegenerateChildren)
{
	// Recover node from CreateSP in FDetailCustomBuilderRow::OnItemNodeInitialized
	const FDetailItemNode* Node = static_cast<const FDetailItemNode*>(InOnRegenerateChildren.GetObjectForTimerManager());
	if (!ensure(Node))
	{
		return;
	}

	WeakNode = Node->AsWeak();
	OnRegenerateChildren = InOnRegenerateChildren;
}

void FVoxelParameterChildBuilder::GenerateHeaderRowContent(FDetailWidgetRow& Row)
{
	ParameterDetails.BuildRow(Row, ValueWidget);
}

void FVoxelParameterChildBuilder::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	if (!ensure(!ParameterDetails.IsOrphan()))
	{
		return;
	}

	const bool bHasAnyOverride = INLINE_LAMBDA
	{
		for (const IVoxelParameterOverridesOwner* Owner : ParameterDetails.OverridesDetails.GetOwners())
		{
			for (const auto& It : Owner->GetPathToValueOverride())
			{
				if (It.Key.StartsWith(ParameterDetails.Path))
				{
					return true;
				}
			}
		}
		return false;
	};

	if (!bIsExpanded &&
		!bHasAnyOverride)
	{
		ChildrenBuilder.AddCustomRow({})
		.NameContent()
		[
			SNew(SVoxelDetailText)
			.Text(INVTEXT("Loading..."))
		];
		return;
	}

	ParameterViewsCommonChildren = FVoxelParameterView::GetCommonChildren(ParameterDetails.ParameterViews);

	FVoxelCategoryBuilder CategoryBuilder(GetName());

	for (const TVoxelArray<FVoxelParameterView*>& ChildParameterViews : ParameterViewsCommonChildren)
	{
		const FString Category = ChildParameterViews[0]->GetParameter().Category;
		for (const FVoxelParameterView* ChildParameterView : ChildParameterViews)
		{
			ensure(ChildParameterView->GetParameter().Category == Category);
		}

		CategoryBuilder.AddProperty(
			Category,
			MakeWeakPtrLambda(this, [=](const FVoxelDetailInterface& DetailInterface)
			{
				ParameterDetails.OverridesDetails.GenerateView(ChildParameterViews, DetailInterface);
			}));
	}

	ParameterDetails.OverridesDetails.AddOrphans(CategoryBuilder, ParameterDetails.Path);
	CategoryBuilder.Apply(ChildrenBuilder);
}

FName FVoxelParameterChildBuilder::GetName() const
{
	return FName(ParameterDetails.Path.ToString());
}