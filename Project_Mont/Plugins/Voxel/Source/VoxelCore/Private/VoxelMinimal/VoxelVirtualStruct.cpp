// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelVirtualStruct);

#if DO_CHECK
VOXEL_RUN_ON_STARTUP_GAME(CheckVoxelVirtualStruct)
{
	for (UScriptStruct* Struct : GetDerivedStructs<FVoxelVirtualStruct>())
	{
		const TSharedRef<FVoxelVirtualStruct> Instance = MakeSharedStruct<FVoxelVirtualStruct>(Struct);

		ensureAlwaysMsgf(Instance->GetStruct() == Struct, TEXT("Missing %s() in %s"),
			*Instance->Internal_GetMacroName(),
			*Struct->GetStructCPPName());
	}
}
#endif

FString FVoxelVirtualStruct::Internal_GetMacroName() const
{
	return "GENERATED_VIRTUAL_STRUCT_BODY";
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVirtualStruct::CopyTo(FVoxelVirtualStruct& Other) const
{
	check(GetStruct() == Other.GetStruct());
	GetStruct()->CopyScriptStruct(&Other, this);
}

TSharedRef<FVoxelVirtualStruct> FVoxelVirtualStruct::MakeSharedCopy() const
{
	return MakeSharedStruct(GetStruct(), this);
}

void FVoxelVirtualStruct::AddStructReferencedObjects(FReferenceCollector& Collector)
{
	FVoxelUtilities::AddStructReferencedObjects(Collector, MakeVoxelStructView(*this));
}

bool FVoxelVirtualStruct::Equals_UPropertyOnly(
	const FVoxelVirtualStruct& Other,
	const bool bIgnoreTransient) const
{
	if (GetStruct() != Other.GetStruct())
	{
		return false;
	}

	for (const FProperty& Property : GetStructProperties(GetStruct()))
	{
		if (bIgnoreTransient &&
			Property.HasAllPropertyFlags(CPF_Transient))
		{
			continue;
		}

		if (!Property.Identical_InContainer(this, &Other))
		{
			return false;
		}
	}

	return true;
}