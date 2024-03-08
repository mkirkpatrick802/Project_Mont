// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Material/VoxelMaterialDefinitionParameterDetails.h"
#include "Customizations/VoxelPinValueCustomizationHelper.h"

FVoxelMaterialDefinitionParameterDetails::FVoxelMaterialDefinitionParameterDetails(
	const FGuid& Guid,
	const FVoxelMaterialDefinitionParameter& Parameter,
	const FSimpleDelegate& RefreshDelegate,
	const TWeakObjectPtr<UVoxelMaterialDefinitionInstance>& MaterialInstance)
	: Guid(Guid)
	, Parameter(Parameter)
	, RefreshDelegate(RefreshDelegate)
	, MaterialInstance(MaterialInstance)
{
	StructOnScope->InitializeAs<FVoxelPinValue>();
	SyncFromViews();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialDefinitionParameterDetails::Tick()
{
	const double Time = FPlatformTime::Seconds();
	if (LastSyncTime + 0.1 < Time)
	{
		LastSyncTime = Time;
		SyncFromViews();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialDefinitionParameterDetails::MakeRow(const FVoxelDetailInterface& DetailInterface)
{
	VOXEL_FUNCTION_COUNTER();

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

	ensure(!StructWrapper);
	StructWrapper = FVoxelPinValueCustomizationHelper::CreatePinValueCustomization(
		PropertyHandle.ToSharedRef(),
		DetailInterface,
		RefreshDelegate,
		Parameter.MetaData,
		[&](FDetailWidgetRow& Row, const TSharedRef<SWidget>& ValueWidget)
		{
			BuildRow(Row, ValueWidget);
		},
		// Used to load/save expansion state
		FAddPropertyParams().UniqueId(FName(Guid.ToString())),
		MakeAttributeLambda(MakeWeakPtrLambda(this, [this]
		{
			return IsEnabled() == ECheckBoxState::Checked;
		})));
}

void FVoxelMaterialDefinitionParameterDetails::BuildRow(
	FDetailWidgetRow& Row,
	const TSharedRef<SWidget>& ValueWidget)
{
	VOXEL_FUNCTION_COUNTER();

	const float Width = FVoxelPinValueCustomizationHelper::GetValueWidgetWidthByType(PropertyHandle, Parameter.Type);

	TSharedRef<SWidget> NameWidget =
		SNew(SVoxelDetailText)
		.ColorAndOpacity(FSlateColor::UseForeground())
		.Text_Lambda(MakeWeakPtrLambda(this, [this]
		{
			return FText::FromName(Parameter.Name);
		}));

	const TAttribute<bool> EnabledAttribute = MakeAttributeLambda(MakeWeakPtrLambda(this, [this]
	{
		return IsEnabled() == ECheckBoxState::Checked;
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

	Row
	.NameContent()
	[
		NameWidget
	]
	.ValueContent()
	.MinDesiredWidth(Width)
	.MaxDesiredWidth(Width)
	[
		ValueWidget
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

ECheckBoxState FVoxelMaterialDefinitionParameterDetails::IsEnabled() const
{
	if (!ensure(MaterialInstance.IsValid()))
	{
		return ECheckBoxState::Checked;
	}

	const FVoxelMaterialParameterValueOverride* ValueOverride = MaterialInstance->GuidToValueOverride.Find(Guid);
	if (!ValueOverride ||
		!ValueOverride->bEnable)
	{
		return ECheckBoxState::Unchecked;
	}

	return ECheckBoxState::Checked;
}

void FVoxelMaterialDefinitionParameterDetails::SetEnabled(const bool bNewEnabled) const
{
	if (!ensure(MaterialInstance.IsValid()))
	{
		return;
	}

	MaterialInstance->PreEditChange(nullptr);

	if (FVoxelMaterialParameterValueOverride* ExistingValueOverride = MaterialInstance->GuidToValueOverride.Find(Guid))
	{
		ExistingValueOverride->bEnable = bNewEnabled;
		ensure(ExistingValueOverride->Value.IsValid());
	}
	else
	{

		FVoxelMaterialParameterValueOverride ValueOverride;
		ValueOverride.bEnable = true;
		ValueOverride.Value = MaterialInstance->GetParameterValue(Guid);
		ensure(ValueOverride.Value.IsValid());

		// Add AFTER doing GetParameterValue so we don't query ourselves
		MaterialInstance->GuidToValueOverride.Add(Guid, ValueOverride);
	}

	MaterialInstance->PostEditChange();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelMaterialDefinitionParameterDetails::CanResetToDefault() const
{
	if (!ensure(MaterialInstance.IsValid()))
	{
		return false;
	}

	const FVoxelMaterialParameterValueOverride* ValueOverride = MaterialInstance->GuidToValueOverride.Find(Guid);
	if (!ValueOverride)
	{
		return false;
	}

	const UVoxelMaterialDefinitionInterface* Parent = MaterialInstance->GetParent();
	if (!Parent)
	{
		return false;
	}

	const FVoxelPinValue ParentValue = Parent->GetParameterValue(Guid);
	if (ParentValue == ValueOverride->Value)
	{
		return false;
	}

	return true;
}

void FVoxelMaterialDefinitionParameterDetails::ResetToDefault()
{
	if (!ensure(MaterialInstance.IsValid()))
	{
		return;
	}

	MaterialInstance->PreEditChange(nullptr);
	ensure(MaterialInstance->GuidToValueOverride.Remove(Guid));
	MaterialInstance->PostEditChange();

	// Do this now as caller will broadcast PostChangeDelegate
	SyncFromViews();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialDefinitionParameterDetails::PreEditChange() const
{
	if (!ensure(MaterialInstance.IsValid()))
	{
		return;
	}

	MaterialInstance->PreEditChange(nullptr);
}

void FVoxelMaterialDefinitionParameterDetails::PostEditChange() const
{
	if (!ensure(MaterialInstance.IsValid()))
	{
		return;
	}

	MaterialInstance->PostEditChange();

	FVoxelMaterialParameterValueOverride& ValueOverride = MaterialInstance->GuidToValueOverride.FindOrAdd(Guid);
	ensure(ValueOverride.bEnable);
	ValueOverride.Value = GetValueRef();

	MaterialInstance->PostEditChange();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialDefinitionParameterDetails::SyncFromViews()
{
	if (!ensure(MaterialInstance.IsValid()))
	{
		return;
	}

	const FVoxelPinValue Value = MaterialInstance->GetParameterValue(Guid);
	if (!ensure(Value.IsValid()))
	{
		return;
	}

	GetValueRef() = Value;
}

FVoxelPinValue& FVoxelMaterialDefinitionParameterDetails::GetValueRef() const
{
	FVoxelPinValue* Value = StructOnScope->Get();
	check(Value);
	return *Value;
}