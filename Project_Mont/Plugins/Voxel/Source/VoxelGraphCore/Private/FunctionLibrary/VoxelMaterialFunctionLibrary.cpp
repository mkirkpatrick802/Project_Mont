// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "FunctionLibrary/VoxelMaterialFunctionLibrary.h"

TVoxelFutureValue<FVoxelComputedMaterialParameter> FVoxelMaterialScalarParameter::Compute(const FVoxelQuery& Query) const
{
	FVoxelComputedMaterialParameter Parameters;
	Parameters.ScalarParameters.Add_CheckNew(Name, Value);
	return Parameters;
}

TVoxelFutureValue<FVoxelComputedMaterialParameter> FVoxelMaterialVectorParameter::Compute(const FVoxelQuery& Query) const
{
	FVoxelComputedMaterialParameter Parameters;
	Parameters.VectorParameters.Add_CheckNew(Name, Value);
	return Parameters;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelMaterialScalarParameter UVoxelMaterialFunctionLibrary::MakeScalarParameter(
	const FName Name,
	const float Value) const
{
	FVoxelMaterialScalarParameter Parameter;
	Parameter.NodeRef = GetNodeRef();
	Parameter.Name = Name;
	Parameter.Value = Value;
	return Parameter;
}

FVoxelMaterialVectorParameter UVoxelMaterialFunctionLibrary::MakeVectorParameter(
	const FName Name,
	const FVector4& Value) const
{
	FVoxelMaterialVectorParameter Parameter;
	Parameter.NodeRef = GetNodeRef();
	Parameter.Name = Name;
	Parameter.Value = Value;
	return Parameter;
}