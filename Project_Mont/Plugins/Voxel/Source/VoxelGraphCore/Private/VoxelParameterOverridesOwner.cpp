// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelParameterOverridesOwner.h"
#include "VoxelGraph.h"
#include "VoxelExposedSeed.h"
#include "VoxelGraphTracker.h"
#include "VoxelParameterView.h"
#include "VoxelGraphParametersView.h"
#include "VoxelGraphParametersViewContext.h"
#include "Buffer/VoxelBaseBuffers.h"

void IVoxelParameterOverridesOwner::NotifyGraphChanged()
{
	VOXEL_FUNCTION_COUNTER();

	FixupParameterOverrides();
	OnParameterLayoutChanged.Broadcast();
}

void IVoxelParameterOverridesOwner::FixupParameterOverrides()
{
	VOXEL_FUNCTION_COUNTER();

	ON_SCOPE_EXIT
	{
		// Always fire, bindee should do a diff to see if anything actually changed
		OnParameterValueChanged.Broadcast();
	};

	UVoxelGraph* Graph = GetGraph();
	TMap<FVoxelParameterPath, FVoxelParameterValueOverride>& PathToValueOverride = GetPathToValueOverride();

	for (auto& It : PathToValueOverride)
	{
		ensure(!It.Value.Value.Is<FVoxelInlineGraph>());
		It.Value.Value.Fixup();
	}

	if (!Graph)
	{
		OnParameterValueChangedPtr.Reset();
#if WITH_EDITOR
		OnParameterChangedPtr.Reset();
#endif
		return;
	}

	{
		OnParameterValueChangedPtr = MakeSharedVoid();

		for (const UVoxelGraph* UsedGraph : Graph->GetUsedGraphs())
		{
			if (UsedGraph == this)
			{
				continue;
			}

			if (UObject* Object = _getUObject())
			{
				UsedGraph->OnParameterValueChanged.Add(MakeWeakPtrDelegate(OnParameterValueChangedPtr, MakeWeakObjectPtrLambda(Object, [this]
				{
					OnParameterValueChanged.Broadcast();
				})));
			}
			else if (const TSharedPtr<IVoxelParameterOverridesOwner> SharedThis = AsShared())
			{
				UsedGraph->OnParameterValueChanged.Add(MakeWeakPtrDelegate(OnParameterValueChangedPtr, MakeWeakPtrLambda(SharedThis, [this]
				{
					OnParameterValueChanged.Broadcast();
				})));
			}
			else
			{
				ensure(false);
			}
		}
	}

#if WITH_EDITOR
	{
		OnParameterChangedPtr = MakeSharedVoid();

		FOnVoxelGraphChanged OnParameterChanged = FOnVoxelGraphChanged::Null();
		if (UObject* Object = _getUObject())
		{
			OnParameterChanged = FOnVoxelGraphChanged::Make(OnParameterChangedPtr, Object, [this]
			{
				FixupParameterOverrides();
				OnParameterLayoutChanged.Broadcast();
			});
		}
		else if (const TSharedPtr<IVoxelParameterOverridesOwner> SharedThis = AsShared())
		{
			OnParameterChanged = FOnVoxelGraphChanged::Make(OnParameterChangedPtr, SharedThis, [this]
			{
				FixupParameterOverrides();
				OnParameterLayoutChanged.Broadcast();
			});
		}
		else
		{
			ensure(false);
		}

		for (const UVoxelGraph* UsedGraph : Graph->GetUsedGraphs())
		{
			GVoxelGraphTracker->OnParameterChanged(*UsedGraph).Add(OnParameterChanged);
		}
	}
#endif

	const TSharedRef<FVoxelGraphParametersView> ParametersView = GetParametersView_ValidGraph();
	for (auto It = PathToValueOverride.CreateIterator(); It; ++It)
	{
		const FVoxelParameterPath Path = It.Key();
		FVoxelParameterValueOverride& ParameterOverrideValue = It.Value();

		if (!ensureVoxelSlow(ParameterOverrideValue.Value.IsValid()))
		{
			It.RemoveCurrent();
			continue;
		}

		if (ShouldForceEnableOverride(Path))
		{
			ParameterOverrideValue.bEnable = true;
		}

		ParametersView->GetContext().AddValueOverrideToIgnore(Path);
		ON_SCOPE_EXIT
		{
			ParametersView->GetContext().RemoveValueOverrideToIgnore();
		};

		// We don't want to store inline graphs overrides, but override of their children
		// using the right parameter path instead
		ensure(!ParameterOverrideValue.Value.Is<FVoxelInlineGraph>());
		ParameterOverrideValue.Value.Fixup();

		const FVoxelParameterView* ParameterView = ParametersView->FindChild(Path);
		if (!ParameterView)
		{
			// Orphan
			continue;
		}

#if WITH_EDITOR
		ParameterOverrideValue.CachedName = ParameterView->GetName();
		ParameterOverrideValue.CachedCategory = ParameterView->GetParameter().Category;
#endif

		if (!ParameterOverrideValue.Value.GetType().CanBeCastedTo(ParameterView->GetType().GetExposedType()))
		{
			FVoxelPinValue NewValue(ParameterView->GetType().GetExposedType());
			if (!NewValue.ImportFromUnrelated(ParameterOverrideValue.Value))
			{
				// Cannot migrate value, create new orphan and reset value to default
				PathToValueOverride.Add(
					Path.GetParent().MakeChild(FGuid::NewGuid()),
					ParameterOverrideValue);
				It.RemoveCurrent();
				continue;
			}

			FVoxelPinValue OldTypeNewValue(ParameterOverrideValue.Value.GetType());
			OldTypeNewValue.ImportFromUnrelated(NewValue);

			if (ParameterOverrideValue.Value.ExportToString() != OldTypeNewValue.ExportToString())
			{
				// Imperfect migration create orphan and still use new value
				PathToValueOverride.Add(
					Path.GetParent().MakeChild(FGuid::NewGuid()),
					ParameterOverrideValue);
			}

			ParameterOverrideValue.Value = NewValue;
		}

		const FVoxelPinValue Value = ParameterView->GetValue();
		if (!ensure(Value.IsValid()))
		{
			continue;
		}

		if (!ShouldForceEnableOverride(Path) &&
			ParameterOverrideValue.bEnable)
		{
			// Explicitly enabled: never reset to default
			continue;
		}

		if (ParameterOverrideValue.Value == Value)
		{
			// Back to default value, no need to store an override
			It.RemoveCurrent();
		}
	}

	// Fixup seeds
	for (const FVoxelParameterView* Child : ParametersView->GetChildren())
	{
		if (!Child->GetType().Is<FVoxelSeed>() ||
			!Child->GetValue().Get<FVoxelExposedSeed>().Seed.IsEmpty())
		{
			continue;
		}

		FVoxelExposedSeed NewSeed;
		NewSeed.Randomize();

		FVoxelParameterValueOverride ValueOverride;
		ValueOverride.bEnable = true;
		ValueOverride.Value = FVoxelPinValue::Make(NewSeed);
		PathToValueOverride.Add(Child->Path, ValueOverride);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FVoxelGraphParametersView> IVoxelParameterOverridesOwner::GetParametersView() const
{
	const UVoxelGraph* Graph = GetGraph();
	if (!Graph)
	{
		return nullptr;
	}

	return GetParametersView_ValidGraph();
}

TSharedRef<FVoxelGraphParametersView> IVoxelParameterOverridesOwner::GetParametersView_ValidGraph() const
{
	VOXEL_FUNCTION_COUNTER();

	const UVoxelGraph* Graph = GetGraph();
	check(Graph);

	const TSharedRef<FVoxelGraphParametersView> ParametersView = MakeVoxelShared<FVoxelGraphParametersView>(*Graph);
	ParametersView->GetContext().SetMainOverridesOwner(ConstCast(this));
	return ParametersView;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool IVoxelParameterOverridesOwner::HasParameter(const FName Name) const
{
	const TSharedPtr<FVoxelGraphParametersView> ParametersView = GetParametersView();
	return
		ParametersView &&
		ParametersView->FindByName(Name) != nullptr;
}

FVoxelPinValue IVoxelParameterOverridesOwner::GetParameter(
	const FName Name,
	FString* OutError) const
{
	VOXEL_FUNCTION_COUNTER();

	const UVoxelGraph* Graph = GetGraph();
	if (!Graph)
	{
		if (OutError)
		{
			*OutError = "Graph is null";
		}
		return {};
	}

	const TSharedRef<FVoxelGraphParametersView> ParametersView = GetParametersView_ValidGraph();
	const FVoxelParameterView* ParameterView = ParametersView->FindByName(Name);
	if (!ParameterView)
	{
		if (OutError)
		{
			TVoxelArray<FString> ValidParameters;
			for (const FGuid& Guid : Graph->GetParameters())
			{
				const FVoxelParameter& Parameter = Graph->FindParameterChecked(Guid);

				ValidParameters.Add(Parameter.Name.ToString() + " (" + Parameter.Type.GetExposedType().ToString() + ")");
			}
			*OutError = "Failed to find " + Name.ToString() + ". Valid parameters: " + FString::Join(ValidParameters, TEXT(", "));
		}
		return {};
	}

	return ParameterView->GetValue();
}

FVoxelPinValue IVoxelParameterOverridesOwner::GetParameterTyped(
	const FName Name,
	const FVoxelPinType& Type,
	FString* OutError) const
{
	const FVoxelPinValue Value = GetParameter(Name, OutError);
	if (!Value.IsValid())
	{
		return {};
	}

	if (!Value.CanBeCastedTo(Type))
	{
		if (OutError)
		{
			*OutError =
				"Parameter " + Name.ToString() + " has type " + Value.GetType().ToString() +
				", but type " + Type.ToString() + " was expected";
		}
		return {};
	}

	return Value;
}

bool IVoxelParameterOverridesOwner::SetParameter(
	const FName Name,
	const FVoxelPinValue& InValue,
	FString* OutError)
{
	VOXEL_FUNCTION_COUNTER();

	const UVoxelGraph* Graph = GetGraph();
	if (!Graph)
	{
		if (OutError)
		{
			*OutError = "Graph is null";
		}
		return false;
	}

	const TSharedRef<FVoxelGraphParametersView> ParametersView = GetParametersView_ValidGraph();
	const FVoxelParameterView* ParameterView = ParametersView->FindByName(Name);
	if (!ParameterView)
	{
		if (OutError)
		{
			TVoxelArray<FString> ValidParameters;
			for (const FGuid& Guid : Graph->GetParameters())
			{
				const FVoxelParameter& Parameter = Graph->FindParameterChecked(Guid);

				ValidParameters.Add(Parameter.Name.ToString() + " (" + Parameter.Type.GetExposedType().ToString() + ")");
			}
			*OutError = "Failed to find " + Name.ToString() + ". Valid parameters: " + FString::Join(ValidParameters, TEXT(", "));
		}
		return false;
	}

	FVoxelPinValue Value = InValue;

	const FVoxelPinType ExposedType = ParameterView->GetType().GetExposedType();
	if (ExposedType.Is<float>() &&
		Value.Is<double>())
	{
		// Implicitly convert from double to float for convenience with blueprints
		Value = FVoxelPinValue::Make<float>(Value.Get<double>());
	}

	if (!Value.CanBeCastedTo(ExposedType))
	{
		if (OutError)
		{
			*OutError =
				"Invalid parameter type for " + Name.ToString() + ". Parameter has type " + ExposedType.ToString() +
				", but value of type " + Value.GetType().ToString() + " was passed";
		}
		return false;
	}

	FVoxelParameterValueOverride ValueOverride;
	ValueOverride.bEnable = true;
	ValueOverride.Value = Value;
	GetPathToValueOverride().Add(ParameterView->Path, ValueOverride);

	FixupParameterOverrides();
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void IVoxelParameterOverridesObjectOwner::PreEditChangeOverrides()
{
	CastChecked<UObject>(this)->PreEditChange(nullptr);
}

void IVoxelParameterOverridesObjectOwner::PostEditChangeOverrides()
{
	CastChecked<UObject>(this)->PostEditChange();
}
#endif

FVoxelParameterOverridesOwnerPtr::FVoxelParameterOverridesOwnerPtr(IVoxelParameterOverridesOwner* Owner)
{
	if (!Owner)
	{
		return;
	}

	OwnerPtr = Owner;

	if (const UObject* Object = Owner->_getUObject())
	{
		ObjectPtr = Object;
	}
	else
	{
		WeakPtr = MakeSharedVoidPtr(Owner->AsShared());
	}

	ensure(IsValid());
}