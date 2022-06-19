#include "AnalyticsWidgetSessions.h"
#include "DataWiseEditor.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h" 
#include "Widgets/Input/SButton.h" 
#include "Widgets/Input/SSearchBox.h" 
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "AnalyticsCapture.h"
#include "AnalyticsWorker.h"
#include "AnalyticsCaptureManager.h"
#include "Async/Async.h"
#include "AnalyticsLocalCaptureManager.h"
#include "EditorStyle.h"


TArray<ValueFilter> ParseFilters(FString filter)
{
	TArray<ValueFilter> filters;

	TArray<FString> filter_strings;
	filter.ParseIntoArray(filter_strings, TEXT(","), true);

	for (FString filter_string : filter_strings)
	{
		bool error = false;
		FString filter_original = filter_string;
		filter_string = filter_string.Replace(TEXT(" "), TEXT(""));

		if (filter_string.MatchesWildcard("*>=*"))
		{
			FString Paramater, Value;
			if (!filter_string.Split(TEXT(">="), &Paramater, &Value)) { error = true; }
			else
			{
				filters.Add(ValueFilter(Paramater, Value, ValueFilter::FILOP_GREATER_EQUALS));
			}
		}
		else if (filter_string.MatchesWildcard("*<=*"))
		{
			FString Paramater, Value;
			if (!filter_string.Split(TEXT("<="), &Paramater, &Value)) { error = true; }
			else
			{
				filters.Add(ValueFilter(Paramater, Value, ValueFilter::FILOP_LESS_EQUALS));
			}
		}
		else if (filter_string.MatchesWildcard("*!=*"))
		{
			FString Paramater, Value;
			if (!filter_string.Split(TEXT("!="), &Paramater, &Value)) { error = true; }
			else
			{
				filters.Add(ValueFilter(Paramater, Value, ValueFilter::FILOP_NOT_EQUALS));
			}
		}
		else if (filter_string.MatchesWildcard("*>*"))
		{
			FString Paramater, Value;
			if (!filter_string.Split(TEXT(">"), &Paramater, &Value)) { error = true; }
			else
			{
				filters.Add(ValueFilter(Paramater, Value, ValueFilter::FILOP_GREATER));
			}

		}
		else if (filter_string.MatchesWildcard("*<*"))
		{
			FString Paramater, Value;
			if (!filter_string.Split(TEXT("<"), &Paramater, &Value)) { error = true; }
			else
			{
				filters.Add(ValueFilter(Paramater, Value, ValueFilter::FILOP_LESS));
			}
		}
		else if (filter_string.MatchesWildcard("*=*"))
		{
			FString Paramater, Value;
			if (!filter_string.Split(TEXT("="), &Paramater, &Value)) { error = true; }
			else
			{
				filters.Add(ValueFilter(Paramater, Value, ValueFilter::FILOP_EQUALS));
			}
		}
		else if (filter_string.MatchesWildcard("!*"))
		{
			filter_string = filter_string.Replace(TEXT("!"), TEXT(""));
			filters.Add(ValueFilter(filter_string, "", ValueFilter::FILOP_NOT_EXIST));
		}
		else if (!filter_string.Contains(">") && !filter_string.Contains("<") && !filter_string.Contains("=") && !filter_string.Contains("!"))
		{
			filters.Add(ValueFilter(filter_string, "", ValueFilter::FILOP_EXIST));
		}
		else
		{
			error = true;
		}

		if (error) UE_LOG(AnalyticsLogEditor, Error, TEXT("Eror parsing filter: %s"), *filter_original);
	}

	return filters;
}






void SWidgetAnalyticsSessions::Construct(const FArguments& InArgs)
{
	ChildSlot[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(5))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SSearchBox)
					.InitialText(FText::FromString(UAnalyticsCaptureManagementTools::GetSessionFilters()))
					.OnTextCommitted(FOnTextCommitted::CreateLambda([this](const FText& text, ETextCommit::Type type)
					{
						if (!text.ToString().Equals(UAnalyticsCaptureManagementTools::GetSessionFilters())) {
							UAnalyticsCaptureManagementTools::SetSessionFilters(text.ToString());
							Filters = ParseFilters(text.ToString());
							RequestSearch();
						}
					}))
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin(3, 0))
				[
					SNew(SButton)
					.IsEnabled_Lambda([this]()
					{
						return status == SearchStatus::Finished;
					})
					.OnClicked_Lambda([this]()
					{
						SelectToggle();
						return FReply::Handled();
					})
					.Text(FText::FromString("(De)select all"))
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.IsEnabled_Lambda([this]()
					{
						return status == SearchStatus::Finished;
					})
					.OnClicked_Lambda([this]()
					{
						OnRequestNewSearch();
						return FReply::Handled();
					})
				.Text(FText::FromString("Refresh"))
				]
				]

			+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(FMargin(5, 0))
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
				[
					SAssignNew(list, SVerticalBox)
				]
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSeparator)
					.Orientation(Orient_Horizontal)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(5))
				[
					SAssignNew(status_label, STextBlock)
				]
		]
	];

	UpdateStatus();
	Filters = ParseFilters(UAnalyticsCaptureManagementTools::GetSessionFilters());
}

void SWidgetAnalyticsSessions::Start()
{
	RequestSearch();

	UAnalyticsCaptureManagementTools::OnConnectionsChanged.AddSP(this, &SWidgetAnalyticsSessions::OnRequestNewSearch);
}

void SWidgetAnalyticsSessions::UpdateList()
{
	int difference = info_list.Num() - list.Get()->GetChildren()->Num();
	int pre_count = list.Get()->GetChildren()->Num();

	if (difference > 0)
	{
		for (int i = 0; i < std::abs(difference); i++) 
		{
			SVerticalBox::FSlot& slot = list->AddSlot();
			TSharedPtr<SHorizontalBox> created_widget;
			SAssignNew(created_widget, SHorizontalBox);

			SHorizontalBox::FSlot& select_slot = created_widget->AddSlot();
			select_slot.AutoWidth();
			select_slot.Padding(FMargin(0, 0, 5, 0));
			TSharedPtr<SCheckBox> select_check;

			int index = pre_count + i;
			SAssignNew(select_check, SCheckBox).OnCheckStateChanged_Lambda([index, this](ECheckBoxState state)
			{
				if (state == ECheckBoxState::Checked)
				{
					this->selection.Add(info_list[index]);
					UAnalyticsCaptureManagementTools::SetSelectedInfos(this->selection);
				}
				else if (state == ECheckBoxState::Unchecked)
				{
					this->selection.Remove(info_list[index]);
					UAnalyticsCaptureManagementTools::SetSelectedInfos(this->selection);
				}

				this->UpdateStatus();
			});
			select_slot.AttachWidget(select_check.ToSharedRef());


			SHorizontalBox::FSlot& label_slot = created_widget->AddSlot();
			label_slot.FillWidth(1.0f);
			TSharedPtr<SVerticalBox> label_container;
			SAssignNew(label_container, SVerticalBox);
			label_slot.AttachWidget(label_container.ToSharedRef());


			SVerticalBox::FSlot& text_name_slot = label_container->AddSlot();
			TSharedPtr<STextBlock> text_name;
			SAssignNew(text_name, STextBlock);
			text_name.Get()->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14));
			text_name_slot.AttachWidget(text_name.ToSharedRef());

			SVerticalBox::FSlot& text_meta_slot = label_container->AddSlot();
			TSharedPtr<STextBlock> text_meta;
			SAssignNew(text_meta, STextBlock);
			text_meta.Get()->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 8));
			text_meta_slot.AttachWidget(text_meta.ToSharedRef());
			text_meta_slot.Padding(FMargin(0, 5, 0, 0));


			slot.Padding(FMargin(0, 0, 0, 5));
			slot.AttachWidget(created_widget.ToSharedRef());
		}
	}
	else if (difference < 0)
	{
		for (int i = 0; i < std::abs(difference); i++)
		{
			list->RemoveSlot(list.Get()->GetChildren()->GetChildAt(list.Get()->GetChildren()->Num() - 1));
		}
	}

	for (int i = 0; i < list.Get()->GetChildren()->Num(); i++)
	{
		SHorizontalBox* entry = static_cast<SHorizontalBox*>(&list.Get()->GetChildren()->GetChildAt(i).Get());
		TSharedRef<SCheckBox> selection_check = StaticCastSharedRef<SCheckBox>(entry->GetChildren()->GetChildAt(0));
		TSharedRef<SVerticalBox> label_container = StaticCastSharedRef<SVerticalBox>(entry->GetChildren()->GetChildAt(1));
		TSharedRef<STextBlock> label_name = StaticCastSharedRef<STextBlock>(label_container->GetChildren()->GetChildAt(0));
		TSharedRef<STextBlock> label_meta = StaticCastSharedRef<STextBlock>(label_container->GetChildren()->GetChildAt(1));

		selection_check->SetIsChecked(selection.Contains(info_list[i]) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
		label_name->SetText(FText::FromString(info_list[i].Name));

		if (info_list[i].Meta.Num() == 0)
		{
			label_meta->SetText(FText::FromString(" "));
		}
		else
		{
			FString str;
			TArray<FString> keys;
			info_list[i].Meta.GetKeys(keys);

			for (int j = 0; j < keys.Num(); j++)
			{
				str += keys[j] + ": " + info_list[i].Meta[keys[j]];
				if (j != keys.Num() - 1) { str += ", "; }
			}

			label_meta->SetText(FText::FromString(str));
		}
	}
}

void SWidgetAnalyticsSessions::OnRequestNewSearch()
{
	if (status == SearchStatus::Finished) RequestSearch();
}

void SWidgetAnalyticsSessions::UpdateSelection()
{
	TArray<FAnalyticsCaptureInfo> found_selection;

	for (FAnalyticsCaptureInfo& info : selection)
	{
		if (info_list.Contains(info)) found_selection.Add(info);
	}

	selection = found_selection;
	UAnalyticsCaptureManagementTools::SetSelectedInfos(this->selection);
}

void SWidgetAnalyticsSessions::UpdateStatus()
{
	switch (status)
	{
	case SearchPending:
	{
		status_label->SetText(FText::FromString("Search task is pending..."));
		break;
	}
	case Searching:
	{
		status_label->SetText(FText::FromString("Searching for sessions..."));
		break;
	}
	case Finished:
	{
		if (info_list.Num() > 0)
		{
			status_label->SetText(FText::FromString(FString::FromInt(selection.Num()) + " / " + FString::FromInt(info_list.Num()) + " selected"));
		}
		else
		{
			status_label->SetText(FText::FromString("No sessions found"));
		}
		break;
	}
	}

}

void SWidgetAnalyticsSessions::SelectToggle()
{
	float balance = (float)selection.Num() / info_list.Num();

	if (balance > 0.5f)
	{
		selection.Empty();
	}
	else
	{
		selection = info_list;
	}

	UpdateList();
	UpdateStatus();
	UAnalyticsCaptureManagementTools::SetSelectedInfos(this->selection);
}



void SWidgetAnalyticsSessions::RequestSearch()
{
	UAnalyticsSearchTask* task = UAnalyticsWorker::Get()->CreateTask<UAnalyticsSearchTask>();
	task->OnSearchStatusUpdated.AddSP(this, &SWidgetAnalyticsSessions::SetStatus);
	task->OnSearchFinish.AddSP(this, &SWidgetAnalyticsSessions::ReceiveInfoList);
	task->Filters = Filters;
	UAnalyticsWorker::Get()->AddTask(task);

	UpdateStatus();
}

void SWidgetAnalyticsSessions::ReceiveInfoList(TArray<FAnalyticsCaptureInfo> info)
{
	if (info_list.Num() == 0)
	{
		info_list = info;
		selection = info_list;
		UAnalyticsCaptureManagementTools::SetSelectedInfos(selection);
	}
	else
	{
		info_list = info;
	}

	status = Finished;
	UpdateList();
	UpdateSelection();
	UpdateStatus();
}

void SWidgetAnalyticsSessions::SetStatus(int stat)
{
	status = static_cast<SearchStatus>(stat);
	UpdateStatus();
}






bool ValueFilter::MatchesFilter(FString Filter, FString Value, FilterOp Operator)
{
	switch (Operator)
	{
	case FILOP_EQUALS: { return Filter.Equals(Value); break; }
	case FILOP_NOT_EQUALS: { return !Filter.Equals(Value); break; }

	case FILOP_GREATER: { return FCString::Atod(*Value) > FCString::Atod(*Filter); break; }
	case FILOP_LESS: { return FCString::Atod(*Value) < FCString::Atod(*Filter); break; }
	case FILOP_GREATER_EQUALS: { return FCString::Atod(*Value) >= FCString::Atod(*Filter); break; }
	case FILOP_LESS_EQUALS: { return FCString::Atod(*Value) <= FCString::Atod(*Filter); break; }
	}

	return false;
}

bool ValueFilter::Matches(FAnalyticsCaptureInfo Info)
{
	if(Operator == FILOP_EXIST)
	{
		return Info.Meta.Contains(Parameter);
	}
	 if(Operator == FILOP_NOT_EXIST)
	{
		 return !Info.Meta.Contains(Parameter);
	}

	if(Info.Meta.Contains(Parameter))
	{
		return MatchesFilter(Filter, Info.Meta[Parameter], Operator);
	}
	else
	{
		return false;
	}
}




bool UAnalyticsSearchTask::Execute()
{

	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		OnSearchStatusUpdated.Broadcast((int)SWidgetAnalyticsSessions::SearchStatus::Searching);
	});

	TArray<UAnalyticsCaptureManagerConnection*> connections = UAnalyticsCaptureManagementTools::GetCaptureManagerConnections();
	connections.Add(UAnalyticsLocalCaptureManager::GetLocalCaptureManager()->Connection.Get());

	TArray<FAnalyticsCaptureInfo> discovered_data;

	for (UAnalyticsCaptureManagerConnection* connection : connections)
	{
		UAnalyticsCaptureManager* manager = connection->GetManager();
		if (manager == nullptr) continue;

		TArray<FAnalyticsCaptureInfo> data = manager->FindCaptures();
		for (FAnalyticsCaptureInfo& info : data)
		{
			bool valid = true;

			for(ValueFilter& filter : Filters)
			{
				if (!filter.Matches(info)) valid = false;
			}

			if(valid) discovered_data.Add(info);
		}

		connection->ReleaseManager();
	}

	// Prevent task from being destroyed before callback has been executed
	finished = false;
	AsyncTask(ENamedThreads::GameThread, [this, &discovered_data]()
	{
		OnSearchFinish.Broadcast(discovered_data);
		finished = true;
	});
	while (!finished) {};

	return true;
}
