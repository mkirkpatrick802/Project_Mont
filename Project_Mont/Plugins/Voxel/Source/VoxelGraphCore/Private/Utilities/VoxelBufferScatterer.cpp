// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Utilities/VoxelBufferScatterer.h"

TSharedRef<const FVoxelBuffer> FVoxelBufferScatterer::Scatter(
	const FVoxelBuffer& BaseBuffer,
	const int32 BaseBufferNum,
	const FVoxelBuffer& BufferToScatter,
	const FVoxelInt32Buffer& Indices)
{
	VOXEL_FUNCTION_COUNTER();
	check(BaseBuffer.GetStruct() == BufferToScatter.GetStruct());

	const TSharedRef<FVoxelBuffer> NewBuffer = FVoxelBuffer::Make(BaseBuffer.GetInnerType());
	check(BaseBuffer.NumTerminalBuffers() == NewBuffer->NumTerminalBuffers());
	check(BufferToScatter.NumTerminalBuffers() == NewBuffer->NumTerminalBuffers());

	for (int32 Index = 0; Index < NewBuffer->NumTerminalBuffers(); Index++)
	{
		Scatter(
			NewBuffer->GetTerminalBuffer(Index),
			BaseBuffer.GetTerminalBuffer(Index),
			BaseBufferNum,
			BufferToScatter.GetTerminalBuffer(Index),
			Indices);
	}
	return NewBuffer;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelBufferScatterer::Scatter(
	FVoxelTerminalBuffer& OutBuffer,
	const FVoxelTerminalBuffer& BaseBuffer,
	const int32 BaseBufferNum,
	const FVoxelTerminalBuffer& BufferToScatter,
	const FVoxelInt32Buffer& Indices)
{
	VOXEL_FUNCTION_COUNTER_NUM(Indices.Num(), 128);
	check(OutBuffer.GetStruct() == BaseBuffer.GetStruct());
	check(OutBuffer.GetStruct() == BufferToScatter.GetStruct());
	check(BaseBuffer.IsConstant() || BaseBuffer.Num() == BaseBufferNum);
	check(Indices.Num() > 0)

	if (Indices.Num() == 0)
	{
		BaseBuffer.CopyTo(OutBuffer);
		return;
	}

	if (OutBuffer.IsA<FVoxelComplexTerminalBuffer>())
	{
		FVoxelComplexTerminalBuffer& OutComplexBuffer = CastChecked<FVoxelComplexTerminalBuffer>(OutBuffer);
		const FVoxelComplexTerminalBuffer& ComplexBaseBuffer = CastChecked<FVoxelComplexTerminalBuffer>(BaseBuffer);
		const FVoxelComplexTerminalBuffer& ComplexBufferToScatter = CastChecked<FVoxelComplexTerminalBuffer>(BufferToScatter);

		const TSharedRef<FVoxelComplexBufferStorage> Storage = OutComplexBuffer.MakeNewStorage();
		Storage->Append(ComplexBaseBuffer.GetStorage(), BaseBufferNum);

		for (int32 ReadIndex = 0; ReadIndex < Indices.Num(); ReadIndex++)
		{
			const int32 WriteIndex = Indices[ReadIndex];
			ComplexBufferToScatter.GetStorage()[ReadIndex].CopyTo((*Storage)[WriteIndex]);
		}

		OutComplexBuffer.SetStorage(Storage);
		return;
	}

	FVoxelSimpleTerminalBuffer& OutSimpleBuffer = CastChecked<FVoxelSimpleTerminalBuffer>(OutBuffer);
	const FVoxelSimpleTerminalBuffer& SimpleBaseBuffer = CastChecked<FVoxelSimpleTerminalBuffer>(BaseBuffer);
	const FVoxelSimpleTerminalBuffer& SimpleBufferToScatter = CastChecked<FVoxelSimpleTerminalBuffer>(BufferToScatter);
	const int32 TypeSize = OutSimpleBuffer.GetTypeSize();

	const TSharedRef<FVoxelBufferStorage> Storage = OutSimpleBuffer.MakeNewStorage();
	Storage->Append(SimpleBaseBuffer.GetStorage(), BaseBufferNum);

	ForeachVoxelBufferChunk_Parallel(Indices.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		const TConstVoxelArrayView<int32> IndicesView = Indices.GetRawView_NotConstant(Iterator);

		VOXEL_SWITCH_TERMINAL_TYPE_SIZE(TypeSize)
		{
			using Type = VOXEL_GET_TYPE(TypeInstance);

			const TVoxelBufferStorage<Type>& ReadStorage = SimpleBufferToScatter.GetStorage<Type>();
			TVoxelBufferStorage<Type>& WriteStorage = Storage->As<Type>();

			if (ReadStorage.IsConstant())
			{
				for (int32 ReadIndex = 0; ReadIndex < Iterator.Num(); ReadIndex++)
				{
					const int32 WriteIndex = IndicesView[ReadIndex];
					WriteStorage[WriteIndex] = ReadStorage[Iterator.GetIndex() + ReadIndex];
				}
			}
			else
			{
				const TConstVoxelArrayView<Type> ReadView = ReadStorage.GetRawView_NotConstant(Iterator);

				for (int32 ReadIndex = 0; ReadIndex < Iterator.Num(); ReadIndex++)
				{
					const int32 WriteIndex = IndicesView[ReadIndex];
					WriteStorage[WriteIndex] = ReadView[ReadIndex];
				}
			}
		};
	});

	OutSimpleBuffer.SetStorage(Storage);
}