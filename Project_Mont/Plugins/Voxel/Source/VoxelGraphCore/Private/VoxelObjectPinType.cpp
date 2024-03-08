// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelObjectPinType.h"

UObject* FVoxelObjectPinType::GetObject(const FConstVoxelStructView Struct) const
{
	const TWeakObjectPtr<UObject> WeakObject = GetWeakObject(Struct);
	ensureVoxelSlow(WeakObject.IsValid() || WeakObject.IsExplicitlyNull());
	return WeakObject.Get();
}

const TVoxelMap<const UScriptStruct*, TSharedPtr<const FVoxelObjectPinType>>& FVoxelObjectPinType::StructToPinType()
{
	static TVoxelMap<const UScriptStruct*, TSharedPtr<const FVoxelObjectPinType>> Map;
	if (Map.Num() == 0)
	{
		VOXEL_FUNCTION_COUNTER();

		for (const UScriptStruct* Struct : GetDerivedStructs<FVoxelObjectPinType>())
		{
			const TSharedRef<FVoxelObjectPinType> Instance = MakeSharedStruct<FVoxelObjectPinType>(Struct);
			Map.Add_CheckNew(Instance->GetStruct(), Instance);
		}

		Map.Shrink();

		GOnVoxelModuleUnloaded_DoCleanup.AddLambda([]
		{
			Map.Empty();
		});
	}
	return Map;
}