// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelSurface.h"

const FName FVoxelSurfaceAttributes::Height = "Height";
const FName FVoxelSurfaceAttributes::Distance = "Distance";
const FName FVoxelSurfaceAttributes::Material = "Material";

TSharedRef<const FVoxelBuffer> FVoxelSurfaceAttributes::MakeDefault(
	const FVoxelPinType& InnerType,
	const FName Attribute)
{
	if (InnerType.Is<float>() &&
		Attribute == Distance)
	{
		return MakeVoxelShared<FVoxelFloatBuffer>(1.e6f);
	}

	return FVoxelBuffer::Make(InnerType);
}

TSharedRef<const FVoxelBuffer> FVoxelSurface::Get(
	const FVoxelPinType& InnerType,
	const FName Attribute) const
{
	ensureMsgf(
		// Don't ensure if we're the default value
		NameToAttribute.Num() == 0 || AttributesToCompute.Contains(Attribute),
		TEXT("Querying %s but it was not in AttributesToCompute!"),
		*Attribute.ToString());

	const TSharedPtr<const FVoxelBuffer> Buffer = NameToAttribute.FindRef(Attribute);
	if (!Buffer)
	{
		return FVoxelSurfaceAttributes::MakeDefault(InnerType, Attribute);
	}

	if (!Buffer->GetInnerType().CanBeCastedTo(InnerType))
	{
		VOXEL_MESSAGE(Error, "Invalid type for attribute {0}: attribute has type {1} but is queried with type {2}",
			Attribute,
			Buffer->GetInnerType().ToString(),
			InnerType.ToString());
		return FVoxelSurfaceAttributes::MakeDefault(InnerType, Attribute);
	}

	return Buffer.ToSharedRef();
}