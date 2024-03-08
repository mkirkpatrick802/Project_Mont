// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

struct FVoxelPinType;

class FVoxelPinValueCustomizationHelper
{
public:
	static TSharedPtr<FVoxelInstancedStructDetailsWrapper> CreatePinValueCustomization(
		const TSharedRef<IPropertyHandle>& PropertyHandle,
		const FVoxelDetailInterface& DetailInterface,
		const FSimpleDelegate& RefreshDelegate,
		const TMap<FName, FString>& MetaData,
		TFunctionRef<void(FDetailWidgetRow&, const TSharedRef<SWidget>&)> SetupRow,
		const FAddPropertyParams& Params,
		const TAttribute<bool>& IsEnabled = true);

	static void CreatePinValueRangeSetter(FDetailWidgetRow& Row,
		const TSharedRef<IPropertyHandle>& PropertyHandle,
		const FText& Name,
		const FText& ToolTip,
		FName Min,
		FName Max,
		const TFunction<bool(const FVoxelPinType&)>& IsVisible,
		const TFunction<TMap<FName, FString>(const TSharedPtr<IPropertyHandle>&)>& GetMetaData,
		const TFunction<void(const TSharedPtr<IPropertyHandle>&, const TMap<FName, FString>&)>& SetMetaData);

	static void CreatePinValueUnitSetter(FDetailWidgetRow& Row,
		const TSharedRef<IPropertyHandle>& PropertyHandle,
		const TFunction<TMap<FName, FString>(const TSharedPtr<IPropertyHandle>&)>& GetMetaData,
		const TFunction<void(const TSharedPtr<IPropertyHandle>&, const TMap<FName, FString>&)>& SetMetaData);

	static void CreatePinValueChannelTypeSetter(FDetailWidgetRow& Row,
		const TSharedRef<IPropertyHandle>& PropertyHandle,
		const TFunction<TMap<FName, FString>(const TSharedPtr<IPropertyHandle>&)>& GetMetaData,
		const TFunction<void(const TSharedPtr<IPropertyHandle>&, const TMap<FName, FString>&)>& SetMetaData);

	static float GetValueWidgetWidthByType(const TSharedPtr<IPropertyHandle>& PropertyHandle, const FVoxelPinType& Type);

	static bool IsNumericType(const FVoxelPinType& Type);

private:
	static TSharedPtr<SWidget> AddArrayItemOptions(const TSharedRef<IPropertyHandle>& PropertyHandle, const TSharedPtr<SWidget>& ValueWidget);
};