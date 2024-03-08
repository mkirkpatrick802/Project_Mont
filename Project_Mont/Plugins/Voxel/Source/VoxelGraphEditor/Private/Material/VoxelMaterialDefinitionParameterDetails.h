// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Material/VoxelMaterialDefinitionInstance.h"
#include "Material/VoxelMaterialDefinitionParameter.h"

class FVoxelMaterialDefinitionParameterDetails : public TSharedFromThis<FVoxelMaterialDefinitionParameterDetails>
{
public:
	const FGuid Guid;
	const FVoxelMaterialDefinitionParameter Parameter;
	const FSimpleDelegate RefreshDelegate;
	const TWeakObjectPtr<UVoxelMaterialDefinitionInstance> MaterialInstance;

	FVoxelMaterialDefinitionParameterDetails(
		const FGuid& Guid,
		const FVoxelMaterialDefinitionParameter& Parameter,
		const FSimpleDelegate& RefreshDelegate,
		const TWeakObjectPtr<UVoxelMaterialDefinitionInstance>& MaterialInstance);

	bool IsValid() const
	{
		return
			ensure(PropertyHandle) &&
			PropertyHandle->IsValidHandle();
	}

public:
	void Tick();

public:
	void MakeRow(const FVoxelDetailInterface& DetailInterface);

	void BuildRow(
		FDetailWidgetRow& Row,
		const TSharedRef<SWidget>& ValueWidget);

public:
	ECheckBoxState IsEnabled() const;
	void SetEnabled(bool bNewEnabled) const;

public:
	bool CanResetToDefault() const;
	void ResetToDefault();

public:
	void PreEditChange() const;
	void PostEditChange() const;

private:
	const TSharedRef<TStructOnScope<FVoxelPinValue>> StructOnScope = MakeVoxelShared<TStructOnScope<FVoxelPinValue>>();

	double LastSyncTime = 0.;
	TSharedPtr<IPropertyHandle> PropertyHandle;
	TSharedPtr<FVoxelInstancedStructDetailsWrapper> StructWrapper;

	void SyncFromViews();
	FVoxelPinValue& GetValueRef() const;
};