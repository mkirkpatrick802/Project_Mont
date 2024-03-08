// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

template<typename T>
class SVoxelGraphPinIntContainer : public SGraphPin
{
public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
	{
		SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
	}

protected:
	//~ Begin SGraphPin Interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override
	{
		const auto CreateNumericTextBox = [&](int32 Component, const FText& Label, const FText& ToolTip)
		{
			return
				SNew(SNumericEntryBox<int32>)
				.LabelVAlign(VAlign_Center)
				.Label()
				[
					SNew(STextBlock)
					.Font(FAppStyle::GetFontStyle("Graph.VectorEditableTextBox"))
					.Text(Label)
					.ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.4f))
				]
				.Value(this, &SVoxelGraphPinIntContainer::GetValue, Component)
				.OnValueCommitted(this, &SVoxelGraphPinIntContainer::OnValueCommitted, Component)
				.Font(FAppStyle::GetFontStyle("Graph.VectorEditableTextBox"))
				.UndeterminedString(INVTEXT("Multiple Values"))
				.ToolTipText(ToolTip)
				.EditableTextBoxStyle(&FAppStyle::GetWidgetStyle<FEditableTextBoxStyle>("Graph.VectorEditableTextBox"))
				.BorderForegroundColor(FLinearColor::White)
				.BorderBackgroundColor(FLinearColor::White);
		};

		TSharedRef<SHorizontalBox> Box =
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.f)
			.HAlign(HAlign_Fill)
			[
				CreateNumericTextBox(0, INVTEXT("X"), INVTEXT("X value"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.f)
			.HAlign(HAlign_Fill)
			[
				CreateNumericTextBox(1, INVTEXT("Y"), INVTEXT("Y value"))
			];

		if constexpr (std::is_same_v<T, FIntVector> || std::is_same_v<T, FIntVector4>)
		{
			Box->AddSlot()
			.AutoWidth()
			.Padding(2.f)
			.HAlign(HAlign_Fill)
			[
				CreateNumericTextBox(2, INVTEXT("Z"), INVTEXT("Z value"))
			];
		}

		if constexpr (std::is_same_v<T, FIntVector4>)
		{
			Box->AddSlot()
			.AutoWidth()
			.Padding(2.f)
			.HAlign(HAlign_Fill)
			[
				CreateNumericTextBox(3, INVTEXT("W"), INVTEXT("W value"))
			];
		}

		return Box;
	}
	//~ End SGraphPin Interface

private:
	TOptional<int32> GetValue(const int32 Component) const
	{
		return GetComponent(GetValue(), Component);
	}

	void OnValueCommitted(const int32 NewValue, ETextCommit::Type CommitType, const int32 Component)
	{
		if (GraphPinObj->IsPendingKill())
		{
			return;
		}

		T CurrentValue = GetValue();
		const int32 TargetOldValue = GetComponent(CurrentValue, Component);

		if (NewValue == TargetOldValue)
		{
			return;
		}

		FVoxelTransaction Transaction(GraphPinObj, "Change Int Vector Pin Value");
		SetComponent(CurrentValue, NewValue, Component);

		const FString NewValueString = FVoxelUtilities::PropertyToText_Direct(
			*FVoxelUtilities::MakeStructProperty(TBaseStructure<T>::Get()),
			reinterpret_cast<void*>(&CurrentValue),
			nullptr);

		GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, NewValueString);
	}

	T GetValue() const
	{
		const FString& DefaultString = GraphPinObj->GetDefaultAsString();

		T Value(0);

		FVoxelUtilities::PropertyFromText_Direct(
			*FVoxelUtilities::MakeStructProperty(TBaseStructure<T>::Get()),
			DefaultString,
			reinterpret_cast<void*>(&Value),
			nullptr);

		return Value;
	}

	static int32 GetComponent(const T& Value, const int32 Component)
	{
		if constexpr (std::is_same_v<T, FIntPoint>)
		{
			switch (Component)
			{
			case 0: return Value.X;
			case 1: return Value.Y;
			default: ensure(false); return 0;
			}
		}
		if constexpr (std::is_same_v<T, FIntVector>)
		{
			switch (Component)
			{
			case 0: return Value.X;
			case 1: return Value.Y;
			case 2: return Value.Z;
			default: ensure(false); return 0;
			}
		}
		if constexpr (std::is_same_v<T, FIntVector4>)
		{
			switch (Component)
			{
			case 0: return Value.X;
			case 1: return Value.Y;
			case 2: return Value.Z;
			case 3: return Value.W;
			default: ensure(false); return 0;
			}
		}

		ensure(false);
		return 0;
	}

	static void SetComponent(T& CurrentValue, const int32 NewComponentValue, const int32 Component)
	{
		if constexpr (std::is_same_v<T, FIntPoint>)
		{
			switch (Component)
			{
			case 0: CurrentValue.X = NewComponentValue; return;
			case 1: CurrentValue.Y = NewComponentValue; return;
			default: ensure(false); return;
			}
		}
		if constexpr (std::is_same_v<T, FIntVector>)
		{
			switch (Component)
			{
			case 0: CurrentValue.X = NewComponentValue; return;
			case 1: CurrentValue.Y = NewComponentValue; return;
			case 2: CurrentValue.Z = NewComponentValue; return;
			default: ensure(false); return;
			}
		}
		if constexpr (std::is_same_v<T, FIntVector4>)
		{
			switch (Component)
			{
			case 0: CurrentValue.X = NewComponentValue; return;
			case 1: CurrentValue.Y = NewComponentValue; return;
			case 2: CurrentValue.Z = NewComponentValue; return;
			case 3: CurrentValue.W = NewComponentValue; return;
			default: ensure(false); return;
			}
		}

		ensure(false);
	}
};