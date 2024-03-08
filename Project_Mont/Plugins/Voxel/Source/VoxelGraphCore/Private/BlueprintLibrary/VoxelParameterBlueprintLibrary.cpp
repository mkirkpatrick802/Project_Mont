// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "BlueprintLibrary/VoxelParameterBlueprintLibrary.h"
#include "VoxelParameterOverridesOwner.h"

DEFINE_FUNCTION(UVoxelParameterBlueprintLibrary::execK2_GetVoxelParameter)
{
	P_GET_TINTERFACE(IVoxelParameterOverridesObjectOwner, Owner);
	P_GET_STRUCT(FName, Name);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);

	P_FINISH;

	if (!ensure(Stack.MostRecentProperty))
	{
		return;
	}

	if (!Owner)
	{
		VOXEL_MESSAGE(Error, "Owner is null");
		return;
	}

	FString Error;
	const FVoxelPinValue Value = Owner->GetParameter(Name, &Error);
	if (!Value.IsValid())
	{
		VOXEL_MESSAGE(Error, "Failed to get parameter: {0}", Error);
		return;
	}

	const FVoxelPinType OutType = FVoxelPinType(*Stack.MostRecentProperty);
	if (!Value.GetType().CanBeCastedTo_K2(OutType))
	{
		VOXEL_MESSAGE(Error, "Parameter {0} has type {1}, but type {2} was expected",
			Name,
			Value.GetType().ToString(),
			OutType.ToString());
		return;
	}

	Value.ExportToProperty(*Stack.MostRecentProperty, Stack.MostRecentPropertyAddress);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_FUNCTION(UVoxelParameterBlueprintLibrary::execK2_SetVoxelParameter)
{
	P_GET_TINTERFACE(IVoxelParameterOverridesObjectOwner, Owner);
	P_GET_STRUCT(FName, Name);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;

	Stack.StepCompiledIn<FProperty>(nullptr);

	const FProperty* Property = Stack.MostRecentProperty;
	const void* PropertyAddress = Stack.MostRecentPropertyAddress;

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;

	// OutValue, manually wired in UK2Node_SetVoxelGraphParameter::ExpandNode
	Stack.StepCompiledIn<FProperty>(nullptr);

	P_FINISH;

	if (!ensure(Property))
	{
		return;
	}

	if (!Owner)
	{
		VOXEL_MESSAGE(Error, "Owner is null");
		return;
	}

	const FVoxelPinValue Value = FVoxelPinValue::MakeFromProperty(*Property, PropertyAddress);

	FString Error;
	if (!Owner->SetParameter(Name, Value, &Error))
	{
		VOXEL_MESSAGE(Error, "{0}", Error);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelParameterBlueprintLibrary::HasVoxelParameter(
	const TScriptInterface<IVoxelParameterOverridesObjectOwner> Owner,
	const FName Name)
{
	if (!Owner)
	{
		VOXEL_MESSAGE(Error, "Owner is null");
		return false;
	}

	return Owner->HasParameter(Name);
}