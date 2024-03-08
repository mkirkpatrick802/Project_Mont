// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelLerpNodes.h"
#include "VoxelCompilationGraph.h"

FVoxelTemplateNode::FPin* FVoxelTemplateNode_LerpBase::ExpandPins(FNode& Node, TArray<FPin*> Pins, const TArray<FPin*>& AllPins) const
{
	const int32 NumPins = Pins.Num();
	const int32 MaxDimension = GetDimension(Node.GetOutputPin(0).Type);

	TArray<FPin*> GenericPins = Filter(Pins, [this](const FPin* Pin)
	{
		return Pin->Name != ClampPin;
	});
	const int32 NumGenericPins = GenericPins.Num();

	TMap<int32, int32> MappedPins;
	for (int32 Index = 0; Index < Pins.Num(); Index++)
	{
		const int32 GenericPinIndex = GenericPins.IndexOfByPredicate([&Pins, &Index](const FPin* Pin)
		{
			return Pin == Pins[Index];
		});

		if (GenericPinIndex == -1)
		{
			continue;
		}

		MappedPins.Add(Index, GenericPinIndex);
	}

	GenericPins = Apply(GenericPins, ConvertToFloat);
	GenericPins = Apply(GenericPins, ScalarToVector, MaxDimension);
	check(GenericPins.Num() == NumGenericPins);

	const TArray<TArray<FPin*>> BrokenPins = ApplyVector(GenericPins, BreakVector);
	check(BrokenPins.Num() == NumGenericPins);

	TArray<TArray<FPin*>> SplitPins;
	for (int32 Index = 0; Index < Pins.Num(); Index++)
	{
		if (const int32* GenericPinIndexPtr = MappedPins.Find(Index))
		{
			SplitPins.Add(BrokenPins[*GenericPinIndexPtr]);
			continue;
		}

		TArray<FPin*> SamePinList;
		for (int32 Dimension = 0; Dimension < MaxDimension; Dimension++)
		{
			SamePinList.Add(Pins[Index]);
		}

		SplitPins.Add(SamePinList);
	}
	check(SplitPins.Num() == NumPins);

	return MakeVector(Call_Multi(GetInnerNode(), SplitPins));
}

#if WITH_EDITOR
FVoxelPinTypeSet FVoxelTemplateNode_LerpBase::GetPromotionTypes(const FVoxelPin& Pin) const
{
	if (Pin.Name == AlphaPin ||
		Pin.Name == ResultPin)
	{
		FVoxelPinTypeSet OutTypes;
		OutTypes.Add(GetFloatTypes());
		return OutTypes;
	}

	FVoxelPinTypeSet OutTypes;
	OutTypes.Add(GetFloatTypes());
	OutTypes.Add(GetIntTypes());
	return OutTypes;
}

void FVoxelTemplateNode_LerpBase::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	// NewType might be int!
	Pin.SetType(NewType);
	ON_SCOPE_EXIT
	{
		ensure(Pin.GetType() == NewType);
	};

	FixupWildcards(NewType);
	EnforceSameDimensions(Pin, NewType, GetAllPins());

	// Fixup Alpha
	SetPinScalarType<float>(GetPin(AlphaPin));

	// Fixup output
	SetPinScalarType<float>(GetPin(ResultPin));

	FixupBuffers(NewType, GetAllPins());
}
#endif