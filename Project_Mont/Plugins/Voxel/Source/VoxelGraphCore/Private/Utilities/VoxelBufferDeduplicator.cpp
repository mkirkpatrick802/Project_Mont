// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Utilities/VoxelBufferDeduplicator.h"

void FVoxelBufferDeduplicator::Deduplicate(
	const FVoxelTerminalBuffer& Buffer,
	FVoxelInt32Buffer& OutIndices,
	FVoxelTerminalBuffer& OutPalette)
{
	VOXEL_FUNCTION_COUNTER_NUM(Buffer.Num(), 128);
	check(Buffer.GetStruct() == OutPalette.GetStruct());

	if (Buffer.Num() == 0)
	{
		OutIndices = FVoxelInt32Buffer::MakeEmpty();
		OutPalette.SetAsEmpty();
		return;
	}
	if (Buffer.IsConstant())
	{
		OutIndices = FVoxelInt32Buffer::Make(0);
		Buffer.CopyTo(OutPalette);
		return;
	}

	if (Buffer.IsA<FVoxelComplexTerminalBuffer>())
	{
		ensure(false);
		return;
	}

	const FVoxelSimpleTerminalBuffer& SimpleBuffer = CastChecked<FVoxelSimpleTerminalBuffer>(Buffer);
	FVoxelSimpleTerminalBuffer& OutSimplePalette = CastChecked<FVoxelSimpleTerminalBuffer>(OutPalette);

	FVoxelInt32BufferStorage Indices;
	Indices.Allocate(Buffer.Num());

	const TSharedRef<FVoxelBufferStorage> Palette = OutSimplePalette.MakeNewStorage();

	VOXEL_SWITCH_TERMINAL_TYPE_SIZE(SimpleBuffer.GetTypeSize())
	{
		using Type = VOXEL_GET_TYPE(TypeInstance);

		TVoxelChunkedMap<Type, int32> ValueToIndex;
		ValueToIndex.Reserve(Buffer.Num());

		for (int32 Index = 0; Index < Buffer.Num(); Index++)
		{
			const Type Value = SimpleBuffer.GetStorage<Type>()[Index];
			const uint32 Hash = ValueToIndex.HashValue(Value);

			if (const int32* ExistingIndex = ValueToIndex.FindHashed(Hash, Value))
			{
				Indices[Index] = *ExistingIndex;
			}
			else
			{
				const int32 NewIndex = Palette->As<Type>().Add(Value);
				ValueToIndex.AddHashed_CheckNew_NoRehash(Hash, Value) = NewIndex;
				Indices[Index] = NewIndex;
			}
		}

		OutIndices.SetStorage(Indices);
		OutSimplePalette.SetStorage(Palette);
	};
}