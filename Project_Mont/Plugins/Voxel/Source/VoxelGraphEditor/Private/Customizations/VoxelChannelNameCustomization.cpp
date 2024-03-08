// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelGraphVisuals.h"
#include "Channel/VoxelChannelName.h"
#include "Channel/VoxelChannelManager.h"
#include "Widgets/SVoxelChannelEditor.h"

class FVoxelChannelNameCustomization : public IPropertyTypeCustomization
{
public:
	virtual void CustomizeHeader(
		const TSharedRef<IPropertyHandle> PropertyHandle,
		FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
		ChannelHandle = PropertyHandle;

		const TSharedRef<IPropertyHandle> NameHandle = PropertyHandle->GetChildHandleStatic(FVoxelChannelName, Name);

		FName Channel;
		switch (NameHandle->GetValue(Channel))
		{
		default:
		{
			ensure(false);
			return;
		}
		case FPropertyAccess::MultipleValues:
		{
			HeaderRow
			.NameContent()
			[
				PropertyHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				NameHandle->CreatePropertyValueWidget()
			];
			return;
		}
		case FPropertyAccess::Fail:
		{
			ensure(false);
			return;
		}
		case FPropertyAccess::Success: break;
		}

		HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.MinDesiredWidth(125.f)
				[
					SAssignNew(SelectedComboButton, SComboButton)
					.ComboButtonStyle(FAppStyle::Get(), "ComboButton")
					.OnGetMenuContent(this, &FVoxelChannelNameCustomization::GetMenuContent)
					.ContentPadding(0)
					.ForegroundColor(FSlateColor::UseForeground())
					.ButtonContent()
					[
						SNew(SHorizontalBox)
						.Clipping(EWidgetClipping::ClipToBoundsAlways)
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						.Padding(0.f, 0.f, 2.f, 0.f)
						.AutoWidth()
						[
							SNew(SImage)
							.Visibility_Lambda(MakeWeakPtrLambda(this, [this]() -> EVisibility
							{
								const TOptional<FVoxelChannelDefinition> ChannelDefinition = GetChannelDefinition();
								return ChannelDefinition.IsSet() && ChannelDefinition->Type.IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
							}))
							.Image_Lambda(MakeWeakPtrLambda(this, [this]() -> const FSlateBrush*
							{
								const TOptional<FVoxelChannelDefinition> ChannelDefinition = GetChannelDefinition();
								if (!ChannelDefinition.IsSet() ||
									!ChannelDefinition->Type.IsValid())
								{
									return nullptr;
								}
								return FVoxelGraphVisuals::GetPinIcon(ChannelDefinition->Type).GetIcon();
							}))
							.ColorAndOpacity_Lambda(MakeWeakPtrLambda(this, [this]() -> FSlateColor
							{
								const TOptional<FVoxelChannelDefinition> ChannelDefinition = GetChannelDefinition();
								if (!ChannelDefinition.IsSet() ||
									!ChannelDefinition->Type.IsValid())
								{
									return FLinearColor::White;
								}
								return FVoxelGraphVisuals::GetPinColor(ChannelDefinition->Type);
							}))
						]
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Left)
						.Padding(2.f, 0.f, 0.f, 0.f)
						.AutoWidth()
						[
							SNew(STextBlock)
							.Font(FVoxelEditorUtilities::Font())
							.Text(FText::FromName(Channel))
						]
					]
				]
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(2.f, 0.f, 0.f, 0.f)
			.AutoWidth()
			[
				SNew(SImage)
				.Visibility_Lambda([this]
				{
					const TOptional<FVoxelChannelDefinition> ChannelDefinition = GetChannelDefinition();
					const FVoxelPinType ChannelType = GetChannelType();

					bool bIsValidChannel = !ChannelType.IsValid() || ChannelType.IsWildcard();
					if (ChannelDefinition.IsSet() &&
						ChannelDefinition->Type.IsValid())
					{
						bIsValidChannel |= ChannelDefinition->Type == ChannelType;
					}

					return bIsValidChannel ? EVisibility::Collapsed : EVisibility::Visible;
				})
				.ToolTipText_Lambda([this]
				{
					return FText::FromString("Selected channel type must be of type " + GetChannelType().ToString());
				})
				.Image(FAppStyle::GetBrush("Icons.Error"))
			]
		];
	}

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
	}

private:
	TSharedRef<SWidget> GetMenuContent()
	{
		if (!ensure(ChannelHandle))
		{
			return SNullWidget::NullWidget;
		}

		const TSharedRef<IPropertyHandle> NameHandle = ChannelHandle->GetChildHandleStatic(FVoxelChannelName, Name);
		FName Channel;
		if (!ensure(NameHandle->GetValue(Channel) == FPropertyAccess::Success))
		{
			return SNullWidget::NullWidget;
		}

		return
			SAssignNew(MenuContent, SMenuOwner)
			[
				SNew(SVoxelChannelEditor)
				.SelectedChannel(Channel)
				.ChannelType(GetChannelType())
				.OnChannelSelected_Lambda(MakeWeakPtrLambda(this, [=](const FName NewChannel)
				{
					NameHandle->SetValue(NewChannel);

					if (MenuContent &&
						SelectedComboButton)
					{
						MenuContent->CloseSummonedMenus();
						SelectedComboButton->SetIsOpen(false);
					}
				}))
			];
	}

	TOptional<FVoxelChannelDefinition> GetChannelDefinition() const
	{
		if (!ensure(ChannelHandle))
		{
			return {};
		}

		const TSharedRef<IPropertyHandle> NameHandle = ChannelHandle->GetChildHandleStatic(FVoxelChannelName, Name);
		FName Channel;
		if (NameHandle->GetValue(Channel) != FPropertyAccess::Success)
		{
			return {};
		}

		return GVoxelChannelManager->FindChannelDefinition(Channel);
	}

	FVoxelPinType GetChannelType() const
	{
		FVoxelPinType Type = FVoxelPinType::Make<FVoxelWildcard>();
		if (!ensure(ChannelHandle))
		{
			return Type;
		}

		if (ChannelHandle->HasMetaData("Type"))
		{
			const FString Value = ChannelHandle->GetMetaData("Type");
			ensure(FVoxelUtilities::TryImportText(Value, Type));
		}
		return Type;
	}

private:
	TSharedPtr<IPropertyHandle> ChannelHandle;
	TSharedPtr<SMenuOwner> MenuContent;
	TSharedPtr<SComboButton> SelectedComboButton;
};

DEFINE_VOXEL_STRUCT_LAYOUT(FVoxelChannelName, FVoxelChannelNameCustomization);