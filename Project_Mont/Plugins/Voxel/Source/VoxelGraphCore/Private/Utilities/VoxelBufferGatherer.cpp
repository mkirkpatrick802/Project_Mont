// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Utilities/VoxelBufferGatherer.h"

TSharedRef<const FVoxelBuffer> FVoxelBufferGatherer::Gather(
	const FVoxelBuffer& Buffer,
	const FVoxelInt32Buffer& Indices)
{
	VOXEL_FUNCTION_COUNTER();

	if (Indices.Num() == 0)
	{
		return FVoxelBuffer::MakeEmpty(Buffer.GetInnerType());
	}
	if (Buffer.IsConstant())
	{
		return Buffer.MakeSharedCopy();
	}

	const TSharedRef<FVoxelBuffer> NewBuffer = FVoxelBuffer::Make(Buffer.GetInnerType());
	check(Buffer.NumTerminalBuffers() == NewBuffer->NumTerminalBuffers());
	for (int32 Index = 0; Index < Buffer.NumTerminalBuffers(); Index++)
	{
		Gather(
			NewBuffer->GetTerminalBuffer(Index),
			Buffer.GetTerminalBuffer(Index),
			Indices);
	}
	return NewBuffer;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelBufferGatherer::Gather(
	FVoxelTerminalBuffer& OutBuffer,
	const FVoxelTerminalBuffer& Buffer,
	const FVoxelInt32Buffer& Indices)
{
	VOXEL_FUNCTION_COUNTER_NUM(Indices.Num(), 128);
	check(OutBuffer.GetStruct() == Buffer.GetStruct());

	if (Buffer.IsA<FVoxelComplexTerminalBuffer>())
	{
		FVoxelComplexTerminalBuffer& OutComplexBuffer = CastChecked<FVoxelComplexTerminalBuffer>(OutBuffer);
		const FVoxelComplexTerminalBuffer& ComplexBuffer = CastChecked<FVoxelComplexTerminalBuffer>(Buffer);

		const TSharedRef<FVoxelComplexBufferStorage> Storage = OutComplexBuffer.MakeNewStorage();
		Storage->Allocate(Indices.Num());

		for (int32 WriteIndex = 0; WriteIndex < Indices.Num(); WriteIndex++)
		{
			const int32 ReadIndex = Indices[WriteIndex];
			ComplexBuffer.GetStorage()[ReadIndex].CopyTo((*Storage)[WriteIndex]);
		}

		OutComplexBuffer.SetStorage(Storage);
		return;
	}

	FVoxelSimpleTerminalBuffer& OutSimpleBuffer = CastChecked<FVoxelSimpleTerminalBuffer>(OutBuffer);
	const FVoxelSimpleTerminalBuffer& SimpleBuffer = CastChecked<FVoxelSimpleTerminalBuffer>(Buffer);
	const int32 TypeSize = SimpleBuffer.GetTypeSize();

	const TSharedRef<FVoxelBufferStorage> Storage = OutSimpleBuffer.MakeNewStorage();
	Storage->Allocate(Indices.Num());

	ForeachVoxelBufferChunk_Parallel(Indices.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		const TConstVoxelArrayView<int32> IndicesView = Indices.GetRawView_NotConstant(Iterator);

		VOXEL_SWITCH_TERMINAL_TYPE_SIZE(TypeSize)
		{
			using Type = VOXEL_GET_TYPE(TypeInstance);

			const TVoxelArrayView<Type> WriteView = Storage->As<Type>().GetRawView_NotConstant(Iterator);
			const TVoxelBufferStorage<Type>& ReadStorage = SimpleBuffer.GetStorage<Type>();

			for (int32 WriteIndex = 0; WriteIndex < Iterator.Num(); WriteIndex++)
			{
				const int32 ReadIndex = IndicesView[WriteIndex];
				WriteView[WriteIndex] = ReadStorage[ReadIndex];
			}
		};
	});

	OutSimpleBuffer.SetStorage(Storage);
}