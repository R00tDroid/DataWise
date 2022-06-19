#include "AnalyticsWidgetConnections.h"
#include "DataWiseEditor.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h" 
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "AnalyticsCaptureManager.h"
#include "ClassFinder.h"
#include "EditorStyle.h"



void SWidgetAnalyticsConnections::Construct(const FArguments& InArgs)
{
	TMap<FString, UClass*> tag_register = UAnalyticsCaptureManagementTools::GetCaptureManagerConnectionTags();

	TArray<FString> tags;
	tag_register.GetKeys(tags);
	for (FString tag : tags)
	{
		manager_types.Add(MakeShareable(new FString(tag)));
	}

	current_type = manager_types[0];

	ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.MinDesiredWidth(200)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
		[
			SAssignNew(entry_headers, SVerticalBox)
		]
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(type_selector, STextComboBox)
			.OptionsSource(&manager_types)
		.OnSelectionChanged(this, &SWidgetAnalyticsConnections::OnAddTypeChanged)
		.InitiallySelectedItem(current_type)
		]

	+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(type_creator, SButton)
			.ToolTipText(FText::FromString("Create new connection"))
			[
				SNew(SImage)
				.Image(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.CreateClassBlueprint").GetIcon())
			]
		]
		]
		]
		]

	+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(4, 0))
		[
			SNew(SSeparator)
			.Orientation(Orient_Vertical)
		]

	+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(content_container, SVerticalBox)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Center)
		[
			SNew(SButton)
			.ToolTipText(FText::FromString("Delete this connection"))
			.OnClicked_Lambda([this]()
			{
				TArray<UAnalyticsCaptureManagerConnection*> entries = UAnalyticsCaptureManagementTools::GetCaptureManagerConnections();
				if (entries.IsValidIndex(this->selected_entry))
				{
					UAnalyticsCaptureManagerConnection* info = entries[this->selected_entry];
					UAnalyticsCaptureManagementTools::UnregisterCaptureManagerConnection(info);
					info->RemoveFromRoot();
					info->ConditionalBeginDestroy();
					UpdateEntryHeaders();
				}
				return FReply::Handled();
			})
			[
				SNew(SImage)
				.Image(FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.AssetActions.Delete").GetIcon())
			]
		]
		]
		]
		]
		];

	type_creator->SetOnClicked(FOnClicked::CreateSP(this, &SWidgetAnalyticsConnections::CreateNewEntry));

	UpdateEntryHeaders();
}

void SWidgetAnalyticsConnections::OnAddTypeChanged(FComboItem NewValue, ESelectInfo::Type)
{
	current_type = NewValue;
}

void SWidgetAnalyticsConnections::UpdateEntryHeaders()
{
	TArray<UAnalyticsCaptureManagerConnection*> entries = UAnalyticsCaptureManagementTools::GetCaptureManagerConnections();

	int difference = entries.Num() - entry_headers.Get()->GetChildren()->Num();
	int pre_count = entry_headers.Get()->GetChildren()->Num();

	if (difference > 0)
	{
		for (int i = 0; i < std::abs(difference); i++) {
			SVerticalBox::FSlot& slot = entry_headers->AddSlot();
			TSharedPtr<SButton> created_widget;
			SAssignNew(created_widget, SButton);
			TSharedPtr<SHorizontalBox> container;
			SAssignNew(container, SHorizontalBox);
			created_widget->SetContent(container.ToSharedRef());

			SHorizontalBox::FSlot& slot_label = container->AddSlot();
			SHorizontalBox::FSlot& slot_info = container->AddSlot();

			slot_label.AutoWidth();
			slot_info.AutoWidth();

			slot_label.HAlign(HAlign_Left);
			slot_info.HAlign(HAlign_Left);

			slot_label.VAlign(VAlign_Center);
			slot_info.VAlign(VAlign_Center);

			slot_info.Padding(FMargin(4, 0, 2, 0));

			TSharedPtr<STextBlock> text_label;
			SAssignNew(text_label, STextBlock);

			TSharedPtr<STextBlock> text_info;
			SAssignNew(text_info, STextBlock);

			text_label.Get()->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12));
			text_info.Get()->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 6));

			slot_label.AttachWidget(text_label.ToSharedRef());
			slot_info.AttachWidget(text_info.ToSharedRef());

			int index = pre_count + i;
			created_widget->SetOnClicked(FOnClicked::CreateLambda([this, index]()
			{
				this->selected_entry = index;
				this->UpdateEntryHeaders();
				this->ShowEntryData();
				return FReply::Handled();
			}));

			slot.AttachWidget(created_widget.ToSharedRef());
		}
	}
	else if (difference < 0)
	{
		for (int i = 0; i < std::abs(difference); i++)
		{
			entry_headers->RemoveSlot(entry_headers.Get()->GetChildren()->GetChildAt(entry_headers.Get()->GetChildren()->Num() - 1));
		}
	}

	int selected_entry_prev = selected_entry;
	selected_entry = FMath::Max(0, FMath::Min(selected_entry, entry_headers.Get()->GetChildren()->Num() - 1));
	if (!entries.IsValidIndex(selected_entry)) selected_entry = -1;

	for (int i = 0; i < entry_headers.Get()->GetChildren()->Num(); i++)
	{
		verify(entries.IsValidIndex(i))

			SButton* entry = static_cast<SButton*>(&entry_headers.Get()->GetChildren()->GetChildAt(i).Get());
		TSharedRef<SWidget> container = entry->GetChildren()->GetChildAt(0);
		TSharedRef<STextBlock> text_label = StaticCastSharedRef<STextBlock>(container->GetChildren()->GetChildAt(0));
		TSharedRef<STextBlock> text_info = StaticCastSharedRef<STextBlock>(container->GetChildren()->GetChildAt(1));

		if (i == selected_entry) {
			entry->SetBorderBackgroundColor(FSlateColor(FLinearColor::Red));
		}
		else {
			entry->SetBorderBackgroundColor(FSlateColor(FLinearColor::Blue));
		}


		text_label->SetText(FText::FromString(entries[i]->GetTag()));
		text_info->SetText(FText::FromString(entries[i]->GetInfo()));
	}

	if (selected_entry_prev != selected_entry) ShowEntryData();
}

FReply SWidgetAnalyticsConnections::CreateNewEntry()
{
	FString selected = *type_selector->GetSelectedItem();
	TMap<FString, UClass*> tag_register = UAnalyticsCaptureManagementTools::GetCaptureManagerConnectionTags();
	UClass** type_ptr = tag_register.Find(selected);

	if (type_ptr != nullptr)
	{
		UClass* type = *type_ptr;

		UAnalyticsCaptureManagerConnection* info = NewObject<UAnalyticsCaptureManagerConnection>((UObject*)GetTransientPackage(), type);
		info->AddToRoot();
		UAnalyticsCaptureManagementTools::RegisterCaptureManagerConnection(info);

		selected_entry = entry_headers.Get()->GetChildren()->Num();
		UpdateEntryHeaders();
		ShowEntryData();
	}


	return FReply::Handled();
}

void SWidgetAnalyticsConnections::ShowEntryData()
{
	while (content_container->GetChildren()->Num() > 0)
	{
		content_container->RemoveSlot(content_container.Get()->GetChildren()->GetChildAt(0));
	}

	TArray<UAnalyticsCaptureManagerConnection*> entries = UAnalyticsCaptureManagementTools::GetCaptureManagerConnections();

	if (entries.IsValidIndex(selected_entry))
	{
		UAnalyticsCaptureManagerConnection* info = entries[selected_entry];
		UClass* type = info->GetClass();

		for (TFieldIterator<UProperty> property_iterator(type); property_iterator; ++property_iterator)
		{
			UProperty* prop = *property_iterator;
			FString type = prop->GetCPPType();
			FString name = prop->GetNameCPP();

			SVerticalBox::FSlot& slot = content_container->AddSlot();

			TSharedPtr<SHorizontalBox> widget_holder;
			SAssignNew(widget_holder, SHorizontalBox);
			slot.Padding(FMargin(0, 0, 0, 20));
			slot.AttachWidget(widget_holder.ToSharedRef());
			slot.AutoHeight();

			SHorizontalBox::FSlot& name_box_slot = widget_holder->AddSlot();
			SHorizontalBox::FSlot& input_slot = widget_holder->AddSlot();
			name_box_slot.Padding(FMargin(0, 0, 10, 0));
			input_slot.Padding(FMargin(0, 0, 2, 0));

			TSharedPtr<SBox> widget_name_box;
			SAssignNew(widget_name_box, SBox);
			widget_name_box->SetMinDesiredWidth(100);
			widget_name_box->SetMaxDesiredWidth(100);
			name_box_slot.AutoWidth();
			name_box_slot.AttachWidget(widget_name_box.ToSharedRef());

			TSharedPtr<STextBlock> widget_name;
			SAssignNew(widget_name, STextBlock);
			widget_name->SetText(name);
			widget_name_box->SetContent(widget_name.ToSharedRef());
			widget_name_box->SetHAlign(HAlign_Right);
			widget_name_box->SetVAlign(VAlign_Center);

			if (type.Equals("FString"))
			{
				FString* data = prop->ContainerPtrToValuePtr<FString>(info);

				TSharedPtr<SEditableTextBox> widget_input;
				SAssignNew(widget_input, SEditableTextBox).OnTextCommitted(FOnTextCommitted::CreateLambda([this, data](const FText& text, ETextCommit::Type CommitInfo)
				{
					*data = text.ToString();
					UAnalyticsCaptureManagementTools::SaveCaptureManagerConnections();
					UAnalyticsCaptureManagementTools::OnConnectionsChanged.Broadcast();
					this->UpdateEntryHeaders();
				}));
				widget_input->SetText(FText::FromString(*data));

				if (prop->GetDisplayNameText().ToString().ToLower().Equals("pwd"))
				{
					widget_input->SetIsPassword(true);
				}

				input_slot.AttachWidget(widget_input.ToSharedRef());
				input_slot.FillWidth(1.0f);
			}
			else if (type.Equals("int32"))
			{
				int32* data = prop->ContainerPtrToValuePtr<int32>(info);

				TSharedPtr<SNumericEntryBox<int32>> widget_input;
				SAssignNew(widget_input, SNumericEntryBox<int32>)
					.Value_Lambda([data]()
				{
					return *data;
				})
					.OnValueChanged_Lambda([data](int32 num)
				{
					*data = num;
				})
					.OnValueCommitted_Lambda([this, data](int32 num, ETextCommit::Type CommitType)
				{
					*data = num;
					UAnalyticsCaptureManagementTools::SaveCaptureManagerConnections();
					UAnalyticsCaptureManagementTools::OnConnectionsChanged.Broadcast();
					this->UpdateEntryHeaders();
				})
					.AllowSpin(false)
					.MinValue(TNumericLimits<int32>::Lowest())
					.MaxValue(TNumericLimits<int32>::Max());

				input_slot.AttachWidget(widget_input.ToSharedRef());
				input_slot.FillWidth(1.0f);
			}
			else if (type.Equals("bool"))
			{
				bool* data = prop->ContainerPtrToValuePtr<bool>(info);

				TSharedPtr<SCheckBox> widget_input;
				SAssignNew(widget_input, SCheckBox).OnCheckStateChanged(FOnCheckStateChanged::CreateLambda([this, data](ECheckBoxState checked)
				{
					*data = checked == ECheckBoxState::Checked;
					UAnalyticsCaptureManagementTools::SaveCaptureManagerConnections();
					UAnalyticsCaptureManagementTools::OnConnectionsChanged.Broadcast();
					this->UpdateEntryHeaders();
				}));

				widget_input->SetIsChecked(*data ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);

				input_slot.AttachWidget(widget_input.ToSharedRef());
				input_slot.AutoWidth();
			}
			else
			{
				UE_LOG(AnalyticsLogEditor, Error, TEXT("Unsupported property type: %s (%s)"), *type, *name);
				break;
			}
		}
	}
}