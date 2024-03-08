// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Customizations/VoxelParameterDetails.h"
#include "Customizations/VoxelParameterChildBuilder.h"
#include "Customizations/VoxelPinValueCustomizationHelper.h"
#include "VoxelGraph.h"
#include "VoxelParameterView.h"
#include "VoxelParameterOverridesDetails.h"
#include "VoxelGraphParametersViewContext.h"

FVoxelParameterDetails::FVoxelParameterDetails(
	FVoxelParameterOverridesDetails& ContainerDetail,
	const FVoxelParameterPath& Path,
	const TVoxelArray<FVoxelParameterView*>& ParameterViews)
	: OverridesDetails(ContainerDetail)
	, Path(Path)
	, ParameterViews(ParameterViews)
{
	StructOnScope->InitializeAs<FVoxelPinValue>();

	for (const IVoxelParameterOverridesOwner* Owner : OverridesDetails.GetOwners())
	{
		bForceEnableOverride = Owner->ShouldForceEnableOverride(Path);
	}
	for (const IVoxelParameterOverridesOwner* Owner : OverridesDetails.GetOwners())
	{
		ensure(bForceEnableOverride == Owner->ShouldForceEnableOverride(Path));
	}

	if (!IsOrphan())
	{
		SyncFromViews();
	}
}

void FVoxelParameterDetails::InitializeOrphan(
	const FVoxelPinValue& Value,
	const bool bNewHasSingleValue)
{
	ensure(IsOrphan());
	ensure(Value.IsValid());

	GetValueRef() = Value;
	bHasSingleValue = bNewHasSingleValue;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelParameterDetails::Tick()
{
	const double Time = FPlatformTime::Seconds();
	if (!IsOrphan() &&
		LastSyncTime + 0.1 < Time)
	{
		LastSyncTime = Time;
		SyncFromViews();
	}

	if (ChildBuilder)
	{
		ChildBuilder->UpdateExpandedState();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelParameterDetails::ShouldRebuildChildren() const
{
	if (!ChildBuilder)
	{
		return false;
	}

	if (ChildBuilder->ParameterViewsCommonChildren.Num() == 0)
	{
		// Not built yet
		return false;
	}

	return
		ChildBuilder->ParameterViewsCommonChildren !=
		FVoxelParameterView::GetCommonChildren(ParameterViews);
}

void FVoxelParameterDetails::RebuildChildren() const
{
	if (!ensure(ChildBuilder))
	{
		return;
	}

	ensure(ChildBuilder->OnRegenerateChildren.ExecuteIfBound());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelParameterDetails::MakeRow(const FVoxelDetailInterface& DetailInterface)
{
	VOXEL_FUNCTION_COUNTER();

	if (!IsOrphan())
	{
		ensure(!RowExposedType.IsValid());
		RowExposedType = ParameterViews[0]->GetType().GetExposedType();
	}
	else
	{
		RowExposedType = OrphanExposedType;
	}

	IDetailPropertyRow* DummyRow = DetailInterface.AddExternalStructure(StructOnScope);
	if (!ensure(DummyRow))
	{
		return;
	}

	DummyRow->Visibility(EVisibility::Collapsed);

	ensure(!PropertyHandle);
	PropertyHandle = DummyRow->GetPropertyHandle();

	if (!ensure(PropertyHandle))
	{
		return;
	}

	FVoxelEditorUtilities::TrackHandle(PropertyHandle);

	const FSimpleDelegate PreChangeDelegate = MakeWeakPtrDelegate(this, [this]
	{
		PreEditChange();
	});
	const FSimpleDelegate PostChangeDelegate = MakeWeakPtrDelegate(this, [this]
	{
		PostEditChange();
	});

	PropertyHandle->SetOnPropertyValuePreChange(PreChangeDelegate);
	PropertyHandle->SetOnPropertyValueChanged(PostChangeDelegate);

	PropertyHandle->SetOnChildPropertyValuePreChange(PreChangeDelegate);
	PropertyHandle->SetOnChildPropertyValueChanged(PostChangeDelegate);

	ensure(!bIsInlineGraph);

	for (const FVoxelParameterView* ParameterView : ParameterViews)
	{
		if (ParameterView->IsInlineGraph())
		{
			bIsInlineGraph = true;
		}
	}

	for (const FVoxelParameterView* ParameterView : ParameterViews)
	{
		ensure(ParameterView->IsInlineGraph() == bIsInlineGraph);
	}

	if (bIsInlineGraph)
	{
		UVoxelGraph* BaseGraph = nullptr;
		if (ParameterViews.Num() > 0)
		{
			const FString BaseGraphPath = ParameterViews[0]->GetParameter().MetaData.FindRef("BaseGraph");
			for (const FVoxelParameterView* ParameterView : ParameterViews)
			{
				ensure(BaseGraphPath == ParameterView->GetParameter().MetaData.FindRef("BaseGraph"));
			}

			if (!BaseGraphPath.IsEmpty())
			{
				BaseGraph = LoadObject<UVoxelGraph>(nullptr, *BaseGraphPath);
				ensureVoxelSlow(BaseGraph);
			}
		}

		// InlineGraph Graph object
		const TSharedRef<IPropertyHandle> ObjectHandle = PropertyHandle->GetChildHandleStatic(FVoxelPinValue, Object);

		const TArray<const UClass*> AllowedClasses = { UVoxelGraph::StaticClass() };

		const TSharedRef<SWidget> ValueWidget =
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(UVoxelGraph::StaticClass())
			.AllowClear(true)
			.PropertyHandle(ObjectHandle)
			.ThumbnailPool(FVoxelEditorUtilities::GetThumbnailPool())
			.OnShouldFilterAsset_Lambda([BaseGraph](const FAssetData& AssetData)
			{
				if (!BaseGraph)
				{
					return false;
				}

				const UVoxelGraph* Graph = CastEnsured<UVoxelGraph>(AssetData.GetAsset());
				if (!ensure(Graph))
				{
					return true;
				}

				return !Graph->GetBaseGraphs().Contains(BaseGraph);
			});

		ensure(!ChildBuilder);
		ChildBuilder = MakeVoxelShared<FVoxelParameterChildBuilder>(*this, ValueWidget);

		DetailInterface.AddCustomBuilder(ChildBuilder.ToSharedRef());
		return;
	}

	bool bMetadataSet = false;
	TMap<FName, FString> MetaData;
	for (const FVoxelParameterView* ParameterView : ParameterViews)
	{
		TMap<FName, FString> NewMetaData = ParameterView->GetParameter().MetaData;
		if (bMetadataSet)
		{
			ensure(MetaData.OrderIndependentCompareEqual(NewMetaData));
		}
		else
		{
			bMetadataSet = true;
			MetaData = MoveTemp(NewMetaData);
		}
	}

	ensure(!StructWrapper);
	StructWrapper = FVoxelPinValueCustomizationHelper::CreatePinValueCustomization(
		PropertyHandle.ToSharedRef(),
		DetailInterface,
		MakeWeakPtrDelegate(&OverridesDetails, [&OverridesDetails = OverridesDetails]
		{
			OverridesDetails.ForceRefresh();
		}),
		MetaData,
		[&](FDetailWidgetRow& Row, const TSharedRef<SWidget>& ValueWidget)
		{
			BuildRow(Row, ValueWidget);
		},
		// Used to load/save expansion state
		FAddPropertyParams().UniqueId(FName(Path.ToString())),
		MakeAttributeLambda(MakeWeakPtrLambda(this, [this]
		{
			return
				bForceEnableOverride ||
				IsOrphan() ||
				IsEnabled() == ECheckBoxState::Checked;
		})));
}

void FVoxelParameterDetails::BuildRow(
	FDetailWidgetRow& Row,
	const TSharedRef<SWidget>& ValueWidget)
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelPinType ExposedType;
	if (ParameterViews.Num() > 0)
	{
		ExposedType = ParameterViews[0]->GetType().GetExposedType();
		for (const FVoxelParameterView* ParameterView : ParameterViews)
		{
			ensure(ExposedType == ParameterView->GetType().GetExposedType());
		}
	}
	else
	{
		ExposedType = OrphanExposedType;
	}

	const float Width = FVoxelPinValueCustomizationHelper::GetValueWidgetWidthByType(PropertyHandle, ExposedType);

	const auto GetRowName = MakeWeakPtrLambda(this, [this]
	{
		if (ParameterViews.Num() == 0)
		{
			return FText::FromName(OrphanName);
		}

		const FName Name = ParameterViews[0]->GetName();
		for (const FVoxelParameterView* ParameterView : ParameterViews)
		{
			ensure(Name == ParameterView->GetName());
		}
		return FText::FromName(Name);
	});

	TSharedRef<SWidget> NameWidget =
		SNew(SVoxelDetailText)
		.ColorAndOpacity(IsOrphan() ? FLinearColor::Red : FSlateColor::UseForeground())
		.Text_Lambda(GetRowName);

	if (!bForceEnableOverride &&
		!IsOrphan())
	{
		const TAttribute<bool> EnabledAttribute = MakeAttributeLambda(MakeWeakPtrLambda(this, [this]
		{
			return
				IsOrphan() ||
				IsEnabled() == ECheckBoxState::Checked;
		}));

		Row.IsEnabled(EnabledAttribute);
		NameWidget->SetEnabled(EnabledAttribute);
		ValueWidget->SetEnabled(EnabledAttribute);

		NameWidget =
			SNew(SVoxelAlwaysEnabledWidget)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked_Lambda(MakeWeakPtrLambda(this, [this]
					{
						return IsEnabled();
					}))
					.OnCheckStateChanged_Lambda(MakeWeakPtrLambda(this, [this](const ECheckBoxState NewState)
					{
						ensure(NewState != ECheckBoxState::Undetermined);
						SetEnabled(NewState == ECheckBoxState::Checked);
					}))
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.FillWidth(1.f)
				[
					NameWidget
				]
			];
	}

	Row
	.FilterString(GetRowName())
	.NameContent()
	[
		NameWidget
	]
	.ValueContent()
	.MinDesiredWidth(Width)
	.MaxDesiredWidth(Width)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SVoxelDetailText)
			.Text(INVTEXT("Multiple Values"))
			.Visibility_Lambda(MakeWeakPtrLambda(this, [this]
			{
				return HasSingleValue() ? EVisibility::Collapsed : EVisibility::Visible;
			}))
		]
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda(MakeWeakPtrLambda(this, [this]
			{
				return HasSingleValue() ? EVisibility::Visible : EVisibility::Collapsed;
			}))
			[
				ValueWidget
			]
		]
	]
	.OverrideResetToDefault(FResetToDefaultOverride::Create(
		MakeAttributeLambda(MakeWeakPtrLambda(this, [this]
		{
			return CanResetToDefault();
		})),
		MakeWeakPtrDelegate(this, [this]
		{
			ResetToDefault();
		}),
		false));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ECheckBoxState FVoxelParameterDetails::IsEnabled() const
{
	ensure(!bForceEnableOverride);
	ensure(!IsOrphan());

	bool bAnyEnabled = false;
	bool bAnyDisabled = false;
	for (const IVoxelParameterOverridesOwner* Owner : OverridesDetails.GetOwners())
	{
		if (const FVoxelParameterValueOverride* ValueOverride = Owner->GetPathToValueOverride().Find(Path))
		{
			if (ValueOverride->bEnable)
			{
				bAnyEnabled = true;
			}
			else
			{
				bAnyDisabled = true;
			}
		}
		else
		{
			bAnyDisabled = true;
		}
	}

	if (bAnyEnabled && !bAnyDisabled)
	{
		return ECheckBoxState::Checked;
	}
	if (!bAnyEnabled && bAnyDisabled)
	{
		return ECheckBoxState::Unchecked;
	}
	return ECheckBoxState::Undetermined;
}

void FVoxelParameterDetails::SetEnabled(const bool bNewEnabled) const
{
	ensure(!bForceEnableOverride);
	ensure(!IsOrphan());

	const TVoxelArray<IVoxelParameterOverridesOwner*> Owners = OverridesDetails.GetOwners();
	if (!ensure(Owners.Num() == ParameterViews.Num()))
	{
		return;
	}

	for (int32 Index = 0; Index < Owners.Num(); Index++)
	{
		IVoxelParameterOverridesOwner& Owner = *Owners[Index];
		Owner.PreEditChangeOverrides();

		if (FVoxelParameterValueOverride* ExistingValueOverride = Owner.GetPathToValueOverride().Find(Path))
		{
			ExistingValueOverride->bEnable = bNewEnabled;
			ensure(ExistingValueOverride->Value.IsValid());
		}
		else
		{
			const FVoxelParameterView* ParameterView = ParameterViews[Index];
			if (!ensure(ParameterView))
			{
				continue;
			}

			FVoxelParameterValueOverride ValueOverride;
			ValueOverride.bEnable = true;
			ValueOverride.Value = ParameterView->GetValue();

			// Add AFTER doing GetValue so we don't query ourselves
			Owner.GetPathToValueOverride().Add(Path, ValueOverride);
		}

		Owner.FixupParameterOverrides();
		Owner.PostEditChangeOverrides();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelParameterDetails::CanResetToDefault() const
{
	if (IsOrphan())
	{
		return true;
	}

	const TVoxelArray<IVoxelParameterOverridesOwner*> Owners = OverridesDetails.GetOwners();
	if (!ensure(Owners.Num() == ParameterViews.Num()))
	{
		return false;
	}

	for (int32 Index = 0; Index < Owners.Num(); Index++)
	{
		IVoxelParameterOverridesOwner& Owner = *Owners[Index];
		const FVoxelParameterView* ParameterView = ParameterViews[Index];
		if (!ensure(ParameterView))
		{
			continue;
		}

		const FVoxelParameterValueOverride* ValueOverride = Owner.GetPathToValueOverride().Find(Path);
		if (!ValueOverride)
		{
			continue;
		}

		ParameterView->Context.AddValueOverrideToIgnore(Path);
		const FVoxelPinValue DefaultValue = ParameterView->GetValue();
		ParameterView->Context.RemoveValueOverrideToIgnore();

		if (ValueOverride->Value != DefaultValue)
		{
			return true;
		}
	}
	return false;
}

void FVoxelParameterDetails::ResetToDefault()
{
	if (IsOrphan())
	{
		for (IVoxelParameterOverridesOwner* Owner : OverridesDetails.GetOwners())
		{
			Owner->PreEditChangeOverrides();
			Owner->GetPathToValueOverride().Remove(Path);
			Owner->FixupParameterOverrides();
			Owner->PostEditChangeOverrides();

			// No need to broadcast OnChanged for orphans
		}

		// Force refresh to remove orphans rows that were removed
		OverridesDetails.ForceRefresh();
		return;
	}

	const TVoxelArray<IVoxelParameterOverridesOwner*> Owners = OverridesDetails.GetOwners();
	if (!ensure(Owners.Num() == ParameterViews.Num()))
	{
		return;
	}

	for (int32 Index = 0; Index < Owners.Num(); Index++)
	{
		IVoxelParameterOverridesOwner& Owner = *Owners[Index];
		const FVoxelParameterView* ParameterView = ParameterViews[Index];
		if (!ensure(ParameterView))
		{
			continue;
		}

		FVoxelParameterValueOverride* ValueOverride = Owner.GetPathToValueOverride().Find(Path);
		if (!ValueOverride)
		{
			// We might be able to only reset to default one of the multi-selected objects
			ensure(OverridesDetails.GetOwners().Num() > 1);
			continue;
		}

		ParameterView->Context.AddValueOverrideToIgnore(Path);
		const FVoxelPinValue DefaultValue = ParameterView->GetValue();
		ParameterView->Context.RemoveValueOverrideToIgnore();

		Owner.PreEditChangeOverrides();
		ValueOverride->Value = DefaultValue;
		Owner.FixupParameterOverrides();
		Owner.PostEditChangeOverrides();

		// Do this now as caller will broadcast PostChangeDelegate
		SyncFromViews();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelParameterDetails::PreEditChange() const
{
	for (IVoxelParameterOverridesOwner* Owner : OverridesDetails.GetOwners())
	{
		Owner->PreEditChangeOverrides();
	}
}

void FVoxelParameterDetails::PostEditChange() const
{
	for (IVoxelParameterOverridesOwner* Owner : OverridesDetails.GetOwners())
	{
		FVoxelParameterValueOverride& ValueOverride = Owner->GetPathToValueOverride().FindOrAdd(Path);
		if (bForceEnableOverride)
		{
			ValueOverride.bEnable = true;
		}
		else
		{
			ensure(ValueOverride.bEnable);
		}

		ValueOverride.Value = GetValueRef();
		Owner->FixupParameterOverrides();
		Owner->PostEditChangeOverrides();
	}

	if (bIsInlineGraph)
	{
		// Inline graph: provider changed, force refresh children
		(void)ChildBuilder->OnRegenerateChildren.ExecuteIfBound();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelParameterDetails::SyncFromViews()
{
	ensure(!IsOrphan());

	bHasSingleValue = true;

	bool bValueIsSet = false;
	FVoxelPinValue Value;
	for (const FVoxelParameterView* ParameterView : ParameterViews)
	{
		FVoxelPinValue NewValue = ParameterView->GetValue();
		if (bValueIsSet)
		{
			if (Value != NewValue)
			{
				bHasSingleValue = false;
			}
		}
		else
		{
			bValueIsSet = true;
			Value = MoveTemp(NewValue);
		}
	}
	ensure(bValueIsSet);

	// Always set value to the first view, otherwise we can't get Type from property handle
	GetValueRef() = Value;
}

FVoxelPinValue& FVoxelParameterDetails::GetValueRef() const
{
	FVoxelPinValue* Value = StructOnScope->Get();
	check(Value);
	return *Value;
}