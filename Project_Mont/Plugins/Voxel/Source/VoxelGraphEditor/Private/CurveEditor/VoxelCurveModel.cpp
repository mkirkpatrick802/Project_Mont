// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelCurveModel.h"

#include "SVoxelCurvePanel.h"
#include "CurveEditorCommands.h"
#include "VoxelCurveEditorUtilities.h"

#include "Factories.h"
#include "UnrealExporter.h"
#include "Exporters/Exporter.h"

#include "Editor/CurveEditor/Private/CurveEditorCopyBuffer.h"

FVoxelCurveModel::FVoxelCurveModel(const TSharedPtr<IPropertyHandle>& Handle, const FSimpleDelegate& OnInvalidate)
		: WeakHandle(Handle), OnInvalidate(OnInvalidate)
{
	Curve = MakeShared<FRichCurve>(FVoxelEditorUtilities::GetStructPropertyValue<FRichCurve>(Handle));

	ClampMin = FVector2D(INT32_MIN);
	ClampMax = FVector2D(INT32_MAX);
	if (Handle->HasMetaData("ClampHorizontalMin"))
	{
		ClampMin.X = FMath::Max(ClampMin.X, Handle->GetFloatMetaData("ClampHorizontalMin"));
	}
	if (Handle->HasMetaData("ClampHorizontalMax"))
	{
		ClampMax.X = FMath::Min(ClampMax.X, Handle->GetFloatMetaData("ClampHorizontalMax"));
	}
	if (Handle->HasMetaData("ClampVerticalMin"))
	{
		ClampMin.Y = FMath::Max(ClampMin.Y, Handle->GetFloatMetaData("ClampVerticalMin"));
	}
	if (Handle->HasMetaData("ClampVerticalMax"))
	{
		ClampMax.Y = FMath::Min(ClampMax.Y, Handle->GetFloatMetaData("ClampVerticalMax"));
	}

	UIMin = FVector2D::Zero();
	UIMax = FVector2D::One();
	if (Handle->HasMetaData("UIHorizontalMin"))
	{
		UIMin.X = FMath::Max(ClampMin.X, Handle->GetFloatMetaData("UIHorizontalMin"));
	}
	if (Handle->HasMetaData("UIHorizontalMax"))
	{
		UIMax.X = FMath::Min(ClampMax.X, Handle->GetFloatMetaData("UIHorizontalMax"));
	}
	if (Handle->HasMetaData("UIVerticalMin"))
	{
		UIMin.Y = FMath::Max(ClampMin.Y, Handle->GetFloatMetaData("UIVerticalMin"));
	}
	if (Handle->HasMetaData("UIVerticalMax"))
	{
		UIMax.Y =FMath::Min(ClampMax.Y,  Handle->GetFloatMetaData("UIVerticalMax"));
	}

	UpdatePanelRanges();
}

void FVoxelCurveModel::UpdatePanelRanges()
{
	if (Curve->GetNumKeys() == 0)
	{
		CurveMin = UIMin;
		CurveMax = UIMax;
		return;
	}

	FVector2D NewCurveMin, NewCurveMax;
	float Min = 0.f;
	float Max = 0.f;
	Curve->GetTimeRange(Min, Max);

	NewCurveMin.X = FMath::Clamp(Min, ClampMin.X, UIMin.X);
	NewCurveMax.X = FMath::Clamp(Max, UIMax.X, ClampMax.X);

	if (bExtendByTangents)
	{
		Curve->GetValueRange(Min, Max);
	}
	else
	{
		Min = 0.f;
		Max = 0.f;

		INLINE_LAMBDA
		{
			if (Curve->Keys.Num() == 0)
			{
				return;
			}

			Min = Max = Curve->Keys[0].Value;
			for (int32 Index = 1; Index < Curve->Keys.Num(); Index++)
			{
				Min = FMath::Min(Min, Curve->Keys[Index].Value);
				Max = FMath::Max(Max, Curve->Keys[Index].Value);
			}
		};
	}

	NewCurveMin.Y = FMath::Clamp(Min, ClampMin.Y, UIMin.Y);
	NewCurveMax.Y = FMath::Clamp(Max, UIMax.Y, ClampMax.Y);

	CurveMin = NewCurveMin;
	CurveMax = NewCurveMax;
}

void FVoxelCurveModel::BindCommands(const TSharedPtr<FUICommandList>& CommandList)
{
	// Interpolation and tangents
	{
		const auto IsSameInterpolationMode = [this](const ERichCurveInterpMode InterpMode)
		{
			return
				CachedKeyAttributes.HasInterpMode() &&
				CachedKeyAttributes.GetInterpMode() == InterpMode;
		};
		
		const auto IsSameTangentMode = [this, IsSameInterpolationMode](const ERichCurveInterpMode InterpMode, const ERichCurveTangentMode TangentMode)
		{
			return
				IsSameInterpolationMode(InterpMode) &&
				CachedKeyAttributes.HasTangentMode() &&
				CachedKeyAttributes.GetTangentMode() == TangentMode;
		};

#if VOXEL_ENGINE_VERSION >= 503
		CommandList->MapAction(FCurveEditorCommands::Get().InterpolationCubicSmartAuto,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::SetKeyAttributes, FKeyAttributes().SetInterpMode(RCIM_Cubic).SetTangentMode(RCTM_SmartAuto)),
			FCanExecuteAction::CreateSP(this, &FVoxelCurveModel::HasSelectedKeys),
			FIsActionChecked::CreateLambda(MakeWeakPtrLambda(this, IsSameTangentMode), RCIM_Cubic, RCTM_SmartAuto));
#endif
		CommandList->MapAction(FCurveEditorCommands::Get().InterpolationCubicAuto,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::SetKeyAttributes, FKeyAttributes().SetInterpMode(RCIM_Cubic).SetTangentMode(RCTM_Auto)),
			FCanExecuteAction::CreateSP(this, &FVoxelCurveModel::HasSelectedKeys),
			FIsActionChecked::CreateLambda(MakeWeakPtrLambda(this, IsSameTangentMode), RCIM_Cubic, RCTM_Auto));
		CommandList->MapAction(FCurveEditorCommands::Get().InterpolationCubicUser,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::SetKeyAttributes, FKeyAttributes().SetInterpMode(RCIM_Cubic).SetTangentMode(RCTM_User)),
			FCanExecuteAction::CreateSP(this, &FVoxelCurveModel::HasSelectedKeys),
			FIsActionChecked::CreateLambda(MakeWeakPtrLambda(this, IsSameTangentMode), RCIM_Cubic, RCTM_User));
		CommandList->MapAction(FCurveEditorCommands::Get().InterpolationCubicBreak,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::SetKeyAttributes, FKeyAttributes().SetInterpMode(RCIM_Cubic).SetTangentMode(RCTM_Break)),
			FCanExecuteAction::CreateSP(this, &FVoxelCurveModel::HasSelectedKeys),
			FIsActionChecked::CreateLambda(MakeWeakPtrLambda(this, IsSameTangentMode), RCIM_Cubic, RCTM_Break));
		CommandList->MapAction(FCurveEditorCommands::Get().InterpolationLinear,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::SetKeyAttributes, FKeyAttributes().SetInterpMode(RCIM_Linear).SetTangentMode(RCTM_Auto)),
			FCanExecuteAction::CreateSP(this, &FVoxelCurveModel::HasSelectedKeys),
			FIsActionChecked::CreateLambda(MakeWeakPtrLambda(this, IsSameInterpolationMode), RCIM_Linear));
		CommandList->MapAction(FCurveEditorCommands::Get().InterpolationConstant,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::SetKeyAttributes, FKeyAttributes().SetInterpMode(RCIM_Constant).SetTangentMode(RCTM_Auto)),
			FCanExecuteAction::CreateSP(this, &FVoxelCurveModel::HasSelectedKeys),
			FIsActionChecked::CreateLambda(MakeWeakPtrLambda(this, IsSameInterpolationMode), RCIM_Constant));
	}


	// Context Menu
	{
		CommandList->MapAction(
			FGenericCommands::Get().Delete,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::DeleteSelectedKeys));

		CommandList->MapAction(
			FGenericCommands::Get().Cut,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::CutSelectedKeys));

		CommandList->MapAction(
			FGenericCommands::Get().Copy,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::CopySelectedKeys));

		CommandList->MapAction(
			FGenericCommands::Get().Paste,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::PasteKeys));

		CommandList->MapAction(
			FCurveEditorCommands::Get().FlattenTangents,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::FlattenTangents));

		CommandList->MapAction(
			FCurveEditorCommands::Get().StraightenTangents,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::StraightenTangents));

		CommandList->MapAction(
			FCurveEditorCommands::Get().SelectAllKeys,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::SelectAllKeys));

		CommandList->MapAction(
			FCurveEditorCommands::Get().DeselectAllKeys,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::DeselectAllKeys));

		CommandList->MapAction(
			FCurveEditorCommands::Get().StepToNextKey,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::SelectNextKey));

		CommandList->MapAction(
			FCurveEditorCommands::Get().StepToPreviousKey,
			FExecuteAction::CreateSP(this, &FVoxelCurveModel::SelectPreviousKey));
	}
}

void FVoxelCurveModel::RefreshCurve()
{
	const TSharedPtr<IPropertyHandle> Handle = WeakHandle.Pin();
	if (!ensure(Handle))
	{
		return;
	}

	*Curve = FVoxelEditorUtilities::GetStructPropertyValue<FRichCurve>(Handle);

	UpdatePanelRanges();
	UpdateCachedKeyAttributes();

	OnInvalidate.ExecuteIfBound();
}

void FVoxelCurveModel::SetCurve(const FRichCurve& InCurve) const
{
	const TSharedPtr<IPropertyHandle> Handle = WeakHandle.Pin();
	if (!ensure(Handle))
	{
		return;
	}

	FVoxelEditorUtilities::SetStructPropertyValue(Handle, InCurve);
	*Curve = InCurve;
	OnInvalidate.ExecuteIfBound();
}

void FVoxelCurveModel::SaveCurve() const
{
	const TSharedPtr<IPropertyHandle> Handle = WeakHandle.Pin();
	if (!ensure(Handle))
	{
		return;
	}

	FVoxelEditorUtilities::SetStructPropertyValue(Handle, *Curve);
	OnInvalidate.ExecuteIfBound();
}

float FVoxelCurveModel::GetHorizontalPosition(const float Input) const
{
	return (Input - CurveMin.X) / (CurveMax.X - CurveMin.X) * (CachedCurveScreenBox.Right - CachedCurveScreenBox.Left) + CachedCurveScreenBox.Left;
}

float FVoxelCurveModel::GetVerticalPosition(const float Output) const
{
	return (1.f - (Output - CurveMin.Y) / (CurveMax.Y - CurveMin.Y)) * (CachedCurveScreenBox.Bottom - CachedCurveScreenBox.Top) + CachedCurveScreenBox.Top;
}

void FVoxelCurveModel::PrepareData(const FGeometry& AllottedGeometry, const SVoxelCurvePanel& Panel)
{
	constexpr float LabelOffsetPixels = 4.f;

	if (Panel.InputGridSpacing.IsSet())
	{
		FVoxelCurveEditorUtilities::CalculateFixedGridSpacing(Panel.InputGridSpacing.GetValue(), FVector2D(CurveMin.X, CurveMax.X), HorizontalSpacing);
	}
	else
	{
		FVoxelCurveEditorUtilities::CalculateAutomaticGridSpacing(AllottedGeometry.GetLocalSize().X, FVector2D(CurveMin.X, CurveMax.X), 60.f, HorizontalSpacing);
	}

	if (Panel.OutputGridSpacing.IsSet())
	{
		FVoxelCurveEditorUtilities::CalculateFixedGridSpacing(Panel.OutputGridSpacing.GetValue(), FVector2D(CurveMin.Y, CurveMax.Y), VerticalSpacing);
	}
	else
	{
		FVoxelCurveEditorUtilities::CalculateAutomaticGridSpacing(AllottedGeometry.GetLocalSize().Y, FVector2D(CurveMin.Y, CurveMax.Y), 30.f, VerticalSpacing);
	}

	CachedCurveScreenBox.Left = VerticalSpacing.MaxLabelSize.X + LabelOffsetPixels * 3.f;
	CachedCurveScreenBox.Right = AllottedGeometry.GetLocalSize().X - LabelOffsetPixels * 2.f - HorizontalSpacing.LastLabelSize.X / 2.f;
	CachedCurveScreenBox.Top = LabelOffsetPixels + VerticalSpacing.LastLabelSize.Y * 0.5f;
	CachedCurveScreenBox.Bottom = AllottedGeometry.GetLocalSize().Y - LabelOffsetPixels * 2.f - HorizontalSpacing.MaxLabelSize.Y;

	const float DisplayRatio = (GetPixelsPerOutput() / GetPixelsPerInput());

	InterpolatedCurvePoints = {};
	InterpolatedCurvePoints.Reserve(Curve->GetNumKeys() + 2);

	InterpolatedCurvePoints.Add(FVector2D(CurveMin.X, Curve->Eval(CurveMin.X)));

	const FKeyHandle FirstKeyHandle = Curve->GetFirstKeyHandle();
	const FKeyHandle LastKeyHandle = Curve->GetLastKeyHandle();

	CachedKeyPoints = {};
	CachedKeyPoints.Reserve(Curve->GetNumKeys());

	for (auto It = Curve->GetKeyHandleIterator(); It; ++It)
	{
		const FRichCurveKey& Key = Curve->GetKey(*It);
		if (Key.Time < CurveMin.X ||
			Key.Time > CurveMax.X)
		{
			continue;
		}

		FVector2D Point = FVector2D(Key.Time, Key.Value);
		InterpolatedCurvePoints.Add(Point);

		FVoxelCurveEditorUtilities::ConvertPointToScreenSpace(Point, CurveMin, CurveMax, CachedCurveScreenBox);

		CachedKeyPoints.Add({ Point, Key.InterpMode, *It });

		if (Key.InterpMode != RCIM_Constant &&
			Key.InterpMode != RCIM_Linear &&
			SelectedKeys.Contains(*It))
		{
			if (*It != FirstKeyHandle)
			{
				CachedKeyPoints.Add({ Point, FVoxelCurveEditorUtilities::GetVectorFromSlopeAndLength(Key.ArriveTangent * -DisplayRatio, -60.f), *It, true });
			}

			if (*It != LastKeyHandle)
			{
				CachedKeyPoints.Add({ Point, FVoxelCurveEditorUtilities::GetVectorFromSlopeAndLength(Key.LeaveTangent * -DisplayRatio, 60.f), *It, false });
			}
		}
	}
	InterpolatedCurvePoints.Add(FVector2D(CurveMax.X, Curve->Eval(CurveMax.X)));

	int32 OldSize = InterpolatedCurvePoints.Num() + 1;
	while (OldSize != InterpolatedCurvePoints.Num())
	{
		FVoxelCurveEditorUtilities::RefineCurvePoints(Curve, CachedCurveScreenBox, ClampMin.Y, ClampMax.Y, InterpolatedCurvePoints);
		OldSize = InterpolatedCurvePoints.Num();
	}

	for (FVector2D& Point : InterpolatedCurvePoints)
	{
		FVoxelCurveEditorUtilities::ConvertPointToScreenSpace(Point, CurveMin, CurveMax, CachedCurveScreenBox);
	}
}

void FVoxelCurveModel::SetKeyAttributes(FKeyAttributes KeyAttributes)
{
	bool bAutoSetTangents = false;
	for (const FKeyHandle& KeyHandle : SelectedKeys)
	{
		if (!Curve->IsKeyHandleValid(KeyHandle))
		{
			continue;
		}

		bAutoSetTangents |= FVoxelCurveEditorUtilities::ApplyAttributesToKey(Curve->GetKey(KeyHandle), KeyAttributes);
	}

	if (bAutoSetTangents)
	{
		Curve->AutoSetTangents();
	}

	UpdatePanelRanges();
	UpdateCachedKeyAttributes();
	SaveCurve();
}

float FVoxelCurveModel::GetPixelsPerInput() const
{
	return (CachedCurveScreenBox.Right - CachedCurveScreenBox.Left) / (CurveMax.X - CurveMin.X);
}

float FVoxelCurveModel::GetPixelsPerOutput() const
{
	return (CachedCurveScreenBox.Bottom - CachedCurveScreenBox.Top) / (CurveMax.Y - CurveMin.Y);
}

FSlateRect FVoxelCurveModel::GetSnapDimensions() const
{
	return FSlateRect(CurveMin.X, CurveMin.Y, CurveMax.X, CurveMax.Y);
}

FVector2D FVoxelCurveModel::GetSnapIntervals() const
{
	return FVector2D(HorizontalSpacing.SliceInterval, VerticalSpacing.SliceInterval);
}

void FVoxelCurveModel::AddKeyAtHoveredPosition(const TWeakPtr<SVoxelCurvePanel> WeakPanel)
{
	const TSharedPtr<SVoxelCurvePanel> Panel = WeakPanel.Pin();
	if (!ensure(Panel))
	{
		return;
	}

	const float Width = CachedCurveScreenBox.Right - CachedCurveScreenBox.Left;
	const float InputAlpha = (Panel->CachedMousePosition.X - CachedCurveScreenBox.Left) / Width;

	float Input = FMath::Lerp(CurveMin.X, CurveMax.X, InputAlpha);
	if (Panel->IsInputSnappingEnabled())
	{
		const int32 TimeMultiplier = FMath::RoundHalfToEven(Input / HorizontalSpacing.SliceInterval);
		Input = TimeMultiplier * HorizontalSpacing.SliceInterval;
	}

	float Output = Curve->Eval(Input);
	if (Panel->IsOutputSnappingEnabled())
	{
		const int32 ValueMultiplier = FMath::RoundHalfToEven(Output / VerticalSpacing.SliceInterval);
		Output = ValueMultiplier * VerticalSpacing.SliceInterval;
	}

	AddKey(Input, Output);
}

void FVoxelCurveModel::AddKey(const float Input, const float Output)
{
	Curve->AddKey(Input, Output);

	UpdatePanelRanges();
	UpdateCachedKeyAttributes();
	SaveCurve();
}

void FVoxelCurveModel::DeleteSelectedKeys()
{
	if (SelectedKeys.Num() == 0)
	{
		return;
	}

	for (const FKeyHandle& KeyHandle : SelectedKeys)
	{
		Curve->DeleteKey(KeyHandle);
	}

	SelectedKeys = {};

	UpdatePanelRanges();
	UpdateCachedKeyAttributes();
	SaveCurve();
}

void FVoxelCurveModel::CutSelectedKeys()
{
	CopySelectedKeys();
	DeleteSelectedKeys();
}

void FVoxelCurveModel::CopySelectedKeys()
{
	const UClass* CopyBufferClass = FindObject<UClass>(nullptr, TEXT("CurveEditor.CurveEditorCopyBuffer"));
	const UClass* CopyableCurveKeysClass = FindObject<UClass>(nullptr, TEXT("CurveEditor.CurveEditorCopyableCurveKeys"));
	if (!ensure(CopyBufferClass) ||
		!ensure(CopyableCurveKeysClass))
	{
		return;
	}

	FStringOutputDevice Archive;
	const FExportObjectInnerContext Context;

	TOptional<double> KeyOffset;

	UCurveEditorCopyBuffer* Buffer = static_cast<UCurveEditorCopyBuffer*>(NewObject<UObject>(GetTransientPackage(), CopyBufferClass, NAME_None, RF_Transient));

	if (SelectedKeys.Num() > 0)
	{
		UCurveEditorCopyableCurveKeys* CopyableCurveKeys = static_cast<UCurveEditorCopyableCurveKeys*>(NewObject<UObject>(Buffer, CopyableCurveKeysClass, NAME_None, RF_Transient));

		CopyableCurveKeys->ShortDisplayName = STATIC_FSTRING("VoxelCurve");
		CopyableCurveKeys->LongDisplayName = STATIC_FSTRING("VoxelCurve");

		for (const FKeyHandle& KeyHandle : SelectedKeys)
		{
			if (!ensure(Curve->IsKeyHandleValid(KeyHandle)))
			{
				continue;
			}

			const FRichCurveKey& Key = Curve->GetKey(KeyHandle);
			CopyableCurveKeys->KeyPositions.Add({ Key.Time, Key.Value });
			CopyableCurveKeys->KeyAttributes.Add(FVoxelCurveEditorUtilities::MakeKeyAttributesFromHandle(*Curve, KeyHandle));

			if (!KeyOffset.IsSet() ||
				Key.Time < KeyOffset.GetValue())
			{
				KeyOffset = Key.Time;
			}
		}

		Buffer->Curves.Add(CopyableCurveKeys);
	}
	else
	{
		UCurveEditorCopyableCurveKeys* CopyableCurveKeys = static_cast<UCurveEditorCopyableCurveKeys*>(NewObject<UObject>(Buffer, CopyableCurveKeysClass, NAME_None, RF_Transient));

		CopyableCurveKeys->ShortDisplayName = STATIC_FSTRING("VoxelCurve");
		CopyableCurveKeys->LongDisplayName = STATIC_FSTRING("VoxelCurve");

		for (auto It = Curve->GetKeyHandleIterator(); It; ++It)
		{
			const FKeyHandle& KeyHandle = *It;
			if (!ensure(Curve->IsKeyHandleValid(KeyHandle)))
			{
				continue;
			}

			const FRichCurveKey& Key = Curve->GetKey(KeyHandle);
			CopyableCurveKeys->KeyPositions.Add({ Key.Time, Key.Value });
			CopyableCurveKeys->KeyAttributes.Add(FVoxelCurveEditorUtilities::MakeKeyAttributesFromHandle(*Curve, KeyHandle));
		}

		Buffer->Curves.Add(CopyableCurveKeys);
	}

	if (KeyOffset.IsSet())
	{
		for (UCurveEditorCopyableCurveKeys* CurveKeys : Buffer->Curves)
		{
			for (int Index = 0; Index < CurveKeys->KeyPositions.Num(); ++Index)
			{
				CurveKeys->KeyPositions[Index].InputValue -= KeyOffset.GetValue();
			}
		}

		Buffer->TimeOffset = KeyOffset.GetValue();
	}
	else
	{
		Buffer->bAbsolutePosition = true;
	}

	UExporter::ExportToOutputDevice(&Context, Buffer, nullptr, Archive, TEXT("copy"), 0, PPF_ExportsNotFullyQualified | PPF_Copy | PPF_Delimited, false, Buffer);
	FPlatformApplicationMisc::ClipboardCopy(*Archive);
}

class FVoxelCurveEditorCopyableCurveKeysObjectTextFactory : public FCustomizableTextObjectFactory
{
public:
	FVoxelCurveEditorCopyableCurveKeysObjectTextFactory()
		: FCustomizableTextObjectFactory(GWarn)
	{
	}

	// FCustomizableTextObjectFactory implementation
	virtual bool CanCreateClass(UClass* InObjectClass, bool& bOmitSubObjs) const override
	{
		const UClass* CopyBufferClass = FindObject<UClass>(nullptr, TEXT("CurveEditor.CurveEditorCopyBuffer"));
		if (!ensure(CopyBufferClass))
		{
			return false;
		}

		if (InObjectClass->IsChildOf(CopyBufferClass))
		{
			return true;
		}
		return false;
	}


	virtual void ProcessConstructedObject(UObject* NewObject) override
	{
		check(NewObject);

		NewCopyBuffers.Add(static_cast<UCurveEditorCopyBuffer*>(NewObject));
	}

public:
	TArray<UCurveEditorCopyBuffer*> NewCopyBuffers;
};

void FVoxelCurveModel::PasteKeys()
{
	// Grab the text to paste from the clipboard
	FString TextToImport;
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	TArray<UCurveEditorCopyBuffer*> ImportedCopyBuffers;

	{
		UPackage* TempPackage = NewObject<UPackage>(nullptr, TEXT("/Engine/Editor/CurveEditor/Transient"), RF_Transient);
		TempPackage->AddToRoot();

		// Turn the text buffer into objects
		FVoxelCurveEditorCopyableCurveKeysObjectTextFactory Factory;
		Factory.ProcessBuffer(TempPackage, RF_Transactional, TextToImport);

		ImportedCopyBuffers = Factory.NewCopyBuffers;

		// Remove the temp package from the root now that it has served its purpose
		TempPackage->RemoveFromRoot();
	}

	ensureMsgf(ImportedCopyBuffers.Num() == 1, TEXT("Multiple copy buffers pasted at one time, only the first one will be used!"));
	UCurveEditorCopyBuffer* SourceBuffer = ImportedCopyBuffers[0];

	if (SourceBuffer->Curves.Num() == 0 ||
		!ensure(SourceBuffer->Curves.Num() == 1))
	{
		return;
	}

	float TimeOffset = SourceBuffer->bAbsolutePosition ? 0.f : SourceBuffer->TimeOffset;

	SelectedKeys = {};

	UCurveEditorCopyableCurveKeys* CurveData = SourceBuffer->Curves[0];
	if (!ensure(CurveData->KeyAttributes.Num() == CurveData->KeyPositions.Num()))
	{
		return;
	}

	TSet<FKeyHandle> NewKeys;
	for (int32 Index = 0; Index < CurveData->KeyPositions.Num(); Index++)
	{
		float InputValue = CurveData->KeyPositions[Index].InputValue + TimeOffset;
		FKeyHandle Handle = Curve->FindKey(InputValue);
		if (Curve->IsKeyHandleValid(Handle))
		{
			FRichCurveKey& Key = Curve->GetKey(Handle);
			Key.Time = InputValue;
			Key.Value = CurveData->KeyPositions[Index].OutputValue;
			FVoxelCurveEditorUtilities::ApplyAttributesToKey(Key, CurveData->KeyAttributes[Index]);
		}
		else
		{
			Handle = Curve->AddKey(InputValue, CurveData->KeyPositions[Index].OutputValue);

			FRichCurveKey& Key = Curve->GetKey(Handle);
			FVoxelCurveEditorUtilities::ApplyAttributesToKey(Key, CurveData->KeyAttributes[Index]);
		}
		NewKeys.Add(Handle);
	}

	SelectedKeys = NewKeys;

	UpdatePanelRanges();
	UpdateCachedKeyAttributes();
	SaveCurve();
}

void FVoxelCurveModel::FlattenTangents()
{
	for (const FKeyHandle& KeyHandle : SelectedKeys)
	{
		if (!Curve->IsKeyHandleValid(KeyHandle))
		{
			continue;
		}

		FRichCurveKey& Key = Curve->GetKey(KeyHandle);
		if (Key.InterpMode == RCIM_Constant ||
			Key.InterpMode == RCIM_Linear)
		{
			continue;
		}

		switch (Key.TangentMode)
		{
		case RCTM_Auto: break;
		UE_503_ONLY(case RCTM_SmartAuto: Key.TangentMode = RCTM_User; break;)
		default: break;
		}

		Key.ArriveTangent = 0.f;
		Key.LeaveTangent = 0.f;
	}

	UpdatePanelRanges();
	UpdateCachedKeyAttributes();
	SaveCurve();
}

void FVoxelCurveModel::StraightenTangents()
{
	const FKeyHandle FirstKeyHandle = Curve->GetFirstKeyHandle();
	const FKeyHandle LastKeyHandle = Curve->GetLastKeyHandle();

	for (const FKeyHandle& KeyHandle : SelectedKeys)
	{
		if (!Curve->IsKeyHandleValid(KeyHandle))
		{
			continue;
		}

		FRichCurveKey& Key = Curve->GetKey(KeyHandle);
		if (Key.InterpMode == RCIM_Constant ||
			Key.InterpMode == RCIM_Linear ||
			KeyHandle == FirstKeyHandle ||
			KeyHandle == LastKeyHandle)
		{
			continue;
		}

		switch (Key.TangentMode)
		{
		case RCTM_Auto: break;
		UE_503_ONLY(case RCTM_SmartAuto: Key.TangentMode = RCTM_User; break;)
		default: break;
		}

		const float NewTangent = (Key.ArriveTangent + Key.LeaveTangent) * 0.5f;
		Key.ArriveTangent = NewTangent;
		Key.LeaveTangent = NewTangent;
	}

	UpdatePanelRanges();
	UpdateCachedKeyAttributes();
	SaveCurve();
}

void FVoxelCurveModel::SelectAllKeys()
{
	TSet<FKeyHandle> Keys;
	for (auto It = Curve->GetKeyHandleIterator(); It; ++It)
	{
		Keys.Add(*It);
	}

	SelectKeys(Keys);
}

void FVoxelCurveModel::DeselectAllKeys()
{
	SelectKeys({});
}

void FVoxelCurveModel::SelectNextKey()
{
	if (SelectedKeys.Num() != 1)
	{
		return;
	}

	FKeyHandle CurrentKeyHandle = *SelectedKeys.CreateIterator();
	if (CurrentKeyHandle == Curve->GetLastKeyHandle())
	{
		SelectedKeys = {};
		return;
	}

	SelectKey(*++SelectedKeys.CreateIterator());
}

void FVoxelCurveModel::SelectPreviousKey()
{
	if (SelectedKeys.Num() != 1)
	{
		return;
	}
}

void FVoxelCurveModel::SelectHoveredKey()
{
}

void FVoxelCurveModel::UpdateCachedKeyAttributes()
{
	TOptional<FKeyAttributes> Attributes;
	ON_SCOPE_EXIT
	{
		CachedKeyAttributes = Attributes.Get({});
	};

	for (const FKeyHandle& KeyHandle : SelectedKeys)
	{
		FKeyAttributes KeyAttributes = FVoxelCurveEditorUtilities::MakeKeyAttributesFromHandle(*Curve, KeyHandle);

		if (!Attributes.IsSet())
		{
			Attributes = KeyAttributes;
		}
		else
		{
			Attributes = FKeyAttributes::MaskCommon(Attributes.Get({}), KeyAttributes);
		}
	}
}

void FVoxelCurveModel::SelectKeys(const TSet<FKeyHandle>& Handles)
{
	SelectedKeys = Handles;
	UpdateCachedKeyAttributes();

	OnInvalidate.ExecuteIfBound();
}

void FVoxelCurveModel::SelectKey(const FKeyHandle& Handle)
{
	SelectedKeys.Add(Handle);
	UpdateCachedKeyAttributes();

	OnInvalidate.ExecuteIfBound();
}

TOptional<float> FVoxelCurveModel::GetSelectedKeysInput() const
{
	TOptional<float> Result;
	for (const FKeyHandle& Key : SelectedKeys)
	{
		if (!Result.IsSet())
		{
			Result = Curve->GetKeyTime(Key);
		}
		else
		{
			const float Input = Curve->GetKeyTime(Key);
			if (Input != *Result)
			{
				Result.Reset();
				break;
			}
		}
	}

	return Result;
}

void FVoxelCurveModel::SetSelectedKeysInput(float NewValue, ETextCommit::Type Type)
{
	NewValue = FMath::Clamp(NewValue, ClampMin.X, ClampMax.X);

	for (const FKeyHandle& Key : SelectedKeys)
	{
		Curve->SetKeyTime(Key, NewValue);
	}

	UpdatePanelRanges();
	UpdateCachedKeyAttributes();
	SaveCurve();
}

TOptional<float> FVoxelCurveModel::GetSelectedKeysOutput() const
{
	TOptional<float> Result;
	for (const FKeyHandle& Key : SelectedKeys)
	{
		if (!Result.IsSet())
		{
			Result = Curve->GetKeyValue(Key);
		}
		else
		{
			const float Input = Curve->GetKeyValue(Key);
			if (Input != *Result)
			{
				Result.Reset();
				break;
			}
		}
	}

	return Result;
}

void FVoxelCurveModel::SetSelectedKeysOutput(float NewValue, const ETextCommit::Type Type)
{
	NewValue = FMath::Clamp(NewValue, ClampMin.Y, ClampMax.Y);

	for (const FKeyHandle& Key : SelectedKeys)
	{
		Curve->SetKeyValue(Key, NewValue);
	}

	UpdatePanelRanges();
	UpdateCachedKeyAttributes();
	SaveCurve();
}

void FVoxelCurveModel::ExtendByTangents()
{
	bExtendByTangents = !bExtendByTangents;
	UpdatePanelRanges();

	OnInvalidate.ExecuteIfBound();
}