// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelParameterOverridesRuntime.h"
#include "VoxelGraph.h"
#include "VoxelGraphEvaluator.h"
#include "VoxelGraphParametersView.h"
#include "VoxelQuery.h"
#include "VoxelDependency.h"
#include "VoxelTerminalGraph.h"
#include "VoxelParameterView.h"
#include "VoxelDependencyTracker.h"
#include "VoxelParameterOverridesOwner.h"
#include "Nodes/VoxelNode_Output.h"

class FVoxelParameterOverridesRuntimesManager : public FVoxelSingleton
{
public:
	TQueue<TWeakPtr<FVoxelParameterOverridesRuntime>, EQueueMode::Mpsc> RuntimesToUpdateQueue;

	//~ Begin FVoxelSingleton Interface
	virtual void Tick() override
	{
		VOXEL_FUNCTION_COUNTER();

		TVoxelSet<TSharedPtr<FVoxelParameterOverridesRuntime>> RuntimesToUpdate;
		{
			TWeakPtr<FVoxelParameterOverridesRuntime> WeakRuntime;
			while (RuntimesToUpdateQueue.Dequeue(WeakRuntime))
			{
				const TSharedPtr<FVoxelParameterOverridesRuntime> Runtime = WeakRuntime.Pin();
				if (!Runtime)
				{
					continue;
				}

				RuntimesToUpdate.Add(Runtime);
			}
		}

		for (const TSharedPtr<FVoxelParameterOverridesRuntime>& Runtime : RuntimesToUpdate)
		{
			Runtime->Update_GameThread();
		}
	}
	//~ End FVoxelSingleton Interface
};
FVoxelParameterOverridesRuntimesManager* GVoxelParameterValuesManager = new FVoxelParameterOverridesRuntimesManager();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelParameterOverridesRuntime);

TSharedRef<FVoxelParameterOverridesRuntime> FVoxelParameterOverridesRuntime::Create(const FVoxelParameterOverridesOwnerPtr& OverridesOwner)
{
	check(IsInGameThread());

	// Will happen if asset is being deleted
	if (!ensureVoxelSlow(OverridesOwner.IsValid()))
	{
		return MakeVoxelShareable(new (GVoxelMemory) FVoxelParameterOverridesRuntime(nullptr));
	}

	const TSharedRef<FVoxelParameterOverridesRuntime> Runtime = MakeVoxelShareable(new (GVoxelMemory) FVoxelParameterOverridesRuntime(OverridesOwner));

	// OnParameterValueChanged is also fired on layout/graph changes
	OverridesOwner->OnParameterValueChanged.Add(MakeWeakPtrDelegate(Runtime, [WeakRuntime = Runtime->AsWeak()]
	{
		GVoxelParameterValuesManager->RuntimesToUpdateQueue.Enqueue(WeakRuntime);
	}));

	// Add top level parameters (ie parameters not in an inline graph)
	// to avoid having to go back to the game thread too often
	if (const UVoxelGraph* Graph = OverridesOwner->GetGraph())
	{
		for (const FGuid& Guid : Graph->GetParameters())
		{
			Runtime->PathToValue_RequiresLock.Add_EnsureNew(FVoxelParameterPath::Make(Guid));
		}
	}

	Runtime->Update_GameThread();
	return Runtime;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFutureValue FVoxelParameterOverridesRuntime::FindParameter(
	const FVoxelPinType& Type,
	const FVoxelParameterPath& Path,
	const FVoxelQuery& Query) const
{
	VOXEL_FUNCTION_COUNTER();
	const FVoxelQueryScope Scope(Query);

	{
		VOXEL_SCOPE_LOCK(CriticalSection);
		if (const FParameterValue* ParameterValue = PathToValue_RequiresLock.Find(Path))
		{
			if (!ensureVoxelSlow(ParameterValue->Value.GetType() == Type))
			{
				VOXEL_MESSAGE(Error, "INTERNAL ERROR: Invalid parameter type: {0} vs {1}",
					Type.ToString(),
					ParameterValue->Value.GetType().ToString());

				return FVoxelRuntimePinValue(Type);
			}

			Query.GetDependencyTracker().AddDependency(ParameterValue->Dependency.ToSharedRef());
			return ParameterValue->Value;
		}
	}

	return
		MakeVoxelTask()
		.RunOnGameThread()
		.Execute(Type, MakeWeakPtrLambda(this, [this, Type, Path, Query]() -> FVoxelFutureValue
		{
			return ConstCast(this)->FindParameter_GameThread(Type, Path, Query);
		}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelRuntimePinValue FVoxelParameterOverridesRuntime::FindParameter_GameThread(
	const FVoxelPinType& Type,
	const FVoxelParameterPath& Path,
	const FVoxelQuery& Query)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	const FVoxelQueryScope Scope(Query);

	if (Query.GetInfo(EVoxelQueryInfo::Local).IsDestroyed())
	{
		// This can happen if a brush runtime is destroyed without the query runtime being destroyed
		// The task is owned by the query runtime & is still run
		return FVoxelRuntimePinValue(Type);
	}

	// Group all the invalidate calls
	const FVoxelDependencyInvalidationScope DependencyInvalidationScope;

	VOXEL_SCOPE_LOCK(CriticalSection);

	// Trigger an update for Path
	// FindOrAdd and not Add because we want to keep the existing value
	PathToValue_RequiresLock.FindOrAdd(Path);

	// Update_GameThread_RequiresLock will only update parameters whose path is in PathToValue_RequiresLock
	Update_GameThread_RequiresLock();

	const FParameterValue* Parameter = PathToValue_RequiresLock.Find(Path);
	if (!Parameter)
	{
		VOXEL_MESSAGE(Error, "INTERNAL ERROR: Missing parameter {0}", Path.ToString());
		return FVoxelRuntimePinValue(Type);
	}

	if (!ensureVoxelSlow(Parameter->Value.GetType() == Type))
	{
		VOXEL_MESSAGE(Error, "INTERNAL ERROR: Invalid parameter type: {0} vs {1}",
			Type.ToString(),
			Parameter->Value.GetType().ToString());

		return FVoxelRuntimePinValue(Type);
	}

	Query.GetDependencyTracker().AddDependency(Parameter->Dependency.ToSharedRef());
	return Parameter->Value;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelParameterOverridesRuntime::Update_GameThread()
{
	// Group all the invalidate calls
	const FVoxelDependencyInvalidationScope DependencyInvalidationScope;
	VOXEL_SCOPE_LOCK(CriticalSection);
	Update_GameThread_RequiresLock();
}

void FVoxelParameterOverridesRuntime::Update_GameThread_RequiresLock()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	check(CriticalSection.IsLocked());

	if (!ensure(OverridesOwner.IsValid()))
	{
		return;
	}

	const TSharedPtr<FVoxelGraphParametersView> ParametersView = OverridesOwner->GetParametersView();
	if (!ParametersView)
	{
		return;
	}

	for (auto It = PathToValue_RequiresLock.CreateIterator(); It; ++It)
	{
		const FVoxelParameterPath& ParameterPath = It.Key();
		FParameterValue& ParameterValue = It.Value();

		const FVoxelParameterView* ParameterView = ParametersView->FindChild(ParameterPath);
		if (!ParameterView)
		{
			if (ParameterValue.Dependency)
			{
				ParameterValue.Dependency->Invalidate();
			}

			It.RemoveCurrent();
			continue;
		}

		const FVoxelPinValue NewValue = ParameterView->GetValue();

		if (!ParameterValue.Dependency)
		{
			ParameterValue.Dependency = FVoxelDependency::Create(
				STATIC_FNAME("Parameter"),
				FName(FString::Printf(TEXT("Path=%s Name=%s Type=%s"),
					*ParameterPath.ToString(),
					*ParameterView->GetName().ToString(),
					*ParameterView->GetType().ToString())));
		}

		if (ParameterView->IsInlineGraph())
		{
			const TSharedRef<FVoxelInlineGraph> InlineGraph = MakeVoxelShared<FVoxelInlineGraph>();
			InlineGraph->Graph = ParameterView->GetValue().Get<UVoxelGraph>();
			InlineGraph->ParameterPath = ParameterPath;

			if (ParameterValue.Value.IsValid() &&
				ensure(ParameterValue.Value.Is<FVoxelInlineGraph>()) &&
				ParameterValue.Value.Get<FVoxelInlineGraph>() == *InlineGraph)
			{
				continue;
			}

			ParameterValue.Value = FVoxelRuntimePinValue::Make(InlineGraph);
			ParameterValue.Dependency->Invalidate();
		}
		else
		{
			if (ParameterValue.Value.IsValid())
			{
				const FVoxelPinValue ExposedValue = FVoxelPinType::MakeExposedValue(
					ParameterValue.Value,
					ParameterValue.Value.GetType().IsBufferArray());

				if (ExposedValue == NewValue)
				{
					continue;
				}
			}

			ParameterValue.Value = FVoxelPinType::MakeRuntimeValue(ParameterView->GetType(), NewValue);
			ensure(ParameterValue.Value.IsValid());

			ParameterValue.Dependency->Invalidate();
		}
	}
}