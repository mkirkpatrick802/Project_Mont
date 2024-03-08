// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelNode_RangeDebug.h"

#include "VoxelNode.h"
#include "VoxelGraph.h"
#include "VoxelNodeStats.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "Utilities/VoxelBufferMathUtilities.h"

#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"

#if WITH_EDITOR
struct FVoxelRangeStats
{
	TVoxelArray<FName> Order;
	TVoxelMap<FName, FFloatInterval> FloatRanges;
	TVoxelMap<FName, FDoubleInterval> DoubleRanges;
	TVoxelMap<FName, FInt32Interval> Int32Ranges;

	FVoxelRangeStats() = default;
	explicit FVoxelRangeStats(const FVoxelBuffer& Buffer, const FName Type)
	{
		VOXEL_FUNCTION_COUNTER();
		Build(Buffer, Type);
	}

	void Union(const FVoxelRangeStats& Other)
	{
		VOXEL_FUNCTION_COUNTER();

		if (!ensure(Order.Num() == Other.Order.Num()))
		{
			return;
		}

		for (const auto& It : Other.FloatRanges)
		{
			if (FFloatInterval* FloatRange = FloatRanges.Find(It.Key))
			{
				FloatRange->Min = FMath::Min(FloatRange->Min, It.Value.Min);
				FloatRange->Max = FMath::Max(FloatRange->Max, It.Value.Max);
			}
		}

		for (const auto& It : Other.DoubleRanges)
		{
			if (FDoubleInterval* DoubleRange = DoubleRanges.Find(It.Key))
			{
				DoubleRange->Min = FMath::Min(DoubleRange->Min, It.Value.Min);
				DoubleRange->Max = FMath::Max(DoubleRange->Max, It.Value.Max);
			}
		}

		for (const auto& It : Other.Int32Ranges)
		{
			if (FInt32Interval* Int32Range = Int32Ranges.Find(It.Key))
			{
				Int32Range->Min = FMath::Min(Int32Range->Min, It.Value.Min);
				Int32Range->Max = FMath::Max(Int32Range->Max, It.Value.Max);
			}
		}
	}

private:
	void Build(const FVoxelBuffer& Buffer, const FName Type)
	{
		if (Buffer.Num() == 0)
		{
			return;
		}

		if (const FVoxelSimpleTerminalBuffer* TerminalBuffer = Buffer.As<FVoxelSimpleTerminalBuffer>())
		{
			ParseRange(*TerminalBuffer, Type);
			return;
		}

		if (const FVoxelQuaternionBuffer* QuaternionBuffer = Buffer.As<FVoxelQuaternionBuffer>())
		{
			const FVoxelVectorBuffer EulerBuffer = FVoxelBufferMathUtilities::MakeEulerFromQuaternion(*QuaternionBuffer);
			ParseRange(EulerBuffer.X, Type.IsNone() ? STATIC_FNAME("Pitch") : Type + TEXTVIEW(".Pitch"));
			ParseRange(EulerBuffer.Y, Type.IsNone() ? STATIC_FNAME("Yaw") : Type + TEXTVIEW(".Yaw"));
			ParseRange(EulerBuffer.Z, Type.IsNone() ? STATIC_FNAME("Roll") : Type + TEXTVIEW(".Roll"));
			return;
		}

		for (const FProperty& Property : GetStructProperties(Buffer.GetStruct()))
		{
			const FVoxelBuffer* ChildBuffer = FVoxelUtilities::TryGetProperty<FVoxelBuffer>(Buffer, Property);
			if (!ChildBuffer)
			{
				continue;
			}

			const FName PropertyName = Property.GetFName();
			const FName NewType = Type.IsNone() ? PropertyName : Type + TEXTVIEW(".") + PropertyName;

			if (const FVoxelSimpleTerminalBuffer* TerminalBuffer = ChildBuffer->As<FVoxelSimpleTerminalBuffer>())
			{
				ParseRange(*TerminalBuffer, NewType);
			}
			else
			{
				Build(*ChildBuffer, NewType);
			}
		}
	}

	void ParseRange(const FVoxelSimpleTerminalBuffer& Buffer, const FName Type)
	{
		if (const FVoxelFloatBuffer* FloatBuffer = Buffer.As<FVoxelFloatBuffer>())
		{
			FloatRanges.Add_CheckNew(Type, FloatBuffer->GetMinMaxSafe());
			Order.Add(Type);
		}
		else if (const FVoxelDoubleBuffer* DoubleBuffer = Buffer.As<FVoxelDoubleBuffer>())
		{
			DoubleRanges.Add_CheckNew(Type, DoubleBuffer->GetMinMaxSafe());
			Order.Add(Type);
		}
		else if (const FVoxelInt32Buffer* Int32Buffer = Buffer.As<FVoxelInt32Buffer>())
		{
			Int32Ranges.Add_CheckNew(Type, Int32Buffer->GetMinMax());
			Order.Add(Type);
		}
	}
};

class FVoxelNodeRangeStatManager
	: public FVoxelSingleton
	, public IVoxelNodeStatProvider
{
public:
	struct FQueuedStat
	{
		FVoxelGraphNodeRef NodeRef;
		FName PinRef;
		TSharedPtr<FVoxelRangeStats> Stats;
	};
	TQueue<FQueuedStat, EQueueMode::Mpsc> Queue;

	TVoxelMap<TWeakObjectPtr<const UEdGraphNode>, TVoxelMap<FEdGraphPinReference, TSharedPtr<FVoxelRangeStats>>> NodeToPinToStats;

	//~ Begin FVoxelSingleton Interface
	virtual void Initialize() override
	{
		GVoxelNodeStatProviders.Add(this);
	}
	virtual void Tick() override
	{
		VOXEL_FUNCTION_COUNTER();

		FQueuedStat QueuedStat;
		while (Queue.Dequeue(QueuedStat))
		{
			UEdGraphNode* GraphNode = QueuedStat.NodeRef.GetGraphNode_EditorOnly();
			if (!ensure(GraphNode))
			{
				continue;
			}

			UEdGraphPin* Pin = GraphNode->FindPin(QueuedStat.PinRef);
			if (!ensure(Pin))
			{
				continue;
			}

			TVoxelMap<FEdGraphPinReference, TSharedPtr<FVoxelRangeStats>>& PinToStats = NodeToPinToStats.FindOrAdd(GraphNode);

			if (const TSharedPtr<FVoxelRangeStats> Stats = PinToStats.FindRef(Pin))
			{
				Stats->Union(*QueuedStat.Stats);
			}
			else
			{
				PinToStats.Add_CheckNew(Pin, QueuedStat.Stats);
			}
		}
	}
	//~ End FVoxelSingleton Interface

	//~ Begin IVoxelNodeStatProvider Interface
	virtual void ClearStats() override
	{
		NodeToPinToStats.Empty();
	}
	virtual bool IsEnabled(const UVoxelGraph& Graph) override
	{
		return Graph.bEnableNodeRangeStats;
	}
	virtual FName GetIconStyleSetName() override
	{
		return STATIC_FNAME("EditorStyle");
	}
	virtual FName GetIconName() override
	{
		return "CurveEd.FitHorizontal";
	}
	virtual FText GetPinText(const UEdGraphPin& Pin) override
	{
		const TVoxelMap<FEdGraphPinReference, TSharedPtr<FVoxelRangeStats>>* PinToStats = NodeToPinToStats.Find(Pin.GetOwningNode());
		if (!PinToStats)
		{
			return {};
		}

		const TSharedPtr<FVoxelRangeStats> Stats = PinToStats->FindRef(&Pin);
		if (!Stats ||
			Stats->Order.Num() == 0)
		{
			return {};
		}

		const auto GetNumberText = [](const auto Value) -> FString
		{
			if (Value >= BIG_NUMBER)
			{
				return FString("Inf");
			}
			else if (Value <= -BIG_NUMBER)
			{
				return FString("-Inf");
			}
			else
			{
				return FText::AsNumber(Value).ToString();
			}
		};

		FString Text = Pin.GetDisplayName().ToString().TrimStartAndEnd();
		for (const FName Key : Stats->Order)
		{
			FString Name = (Text.IsEmpty() ? "" : "\n") + (Key.IsNone() ? "" : Key.ToString() + ": ");
			if (const FFloatInterval* FloatRange = Stats->FloatRanges.Find(Key))
			{
				Text += Name + GetNumberText(FloatRange->Min) + " to " + GetNumberText(FloatRange->Max);
			}
			else if (const FDoubleInterval* DoubleRange = Stats->DoubleRanges.Find(Key))
			{
				Text += Name + GetNumberText(DoubleRange->Min) + " to " + GetNumberText(DoubleRange->Max);
			}
			else if (const FInt32Interval* Int32Range = Stats->Int32Ranges.Find(Key))
			{
				Text += Name + FText::AsNumber(Int32Range->Min).ToString() + " to " + FText::AsNumber(Int32Range->Max).ToString();
			}
		}

		return FText::FromString(Text);
	}
	//~ End IVoxelNodeStatProvider Interface

	void EnqueueStats(const IVoxelNodeInterface& InNode, const FName PinName, const TSharedRef<FVoxelRangeStats>& Stats)
	{
		Queue.Enqueue(
		{
			InNode.GetNodeRef(),
			PinName,
			Stats
		});
	}
};
FVoxelNodeRangeStatManager* GVoxelNodeRangeStatManager = new FVoxelNodeRangeStatManager();
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_RangeDebug, Out)
{
#if WITH_EDITOR
	const FVoxelPinType InType = GetNodeRuntime().GetPinData(InPin).Type;

	if (InType.IsBuffer())
	{
		const TValue<FVoxelBuffer> Value = Get<FVoxelBuffer>(InPin, Query);
		return VOXEL_ON_COMPLETE(Value, InType)
		{
			GVoxelNodeRangeStatManager->EnqueueStats(*this, RefPin, MakeShared<FVoxelRangeStats>(*Value, ""));
			return FVoxelRuntimePinValue::Make(Value, InType);
		};
	}
#endif

	ensure(GIsEditor);
	return Get(InPin, Query);
}