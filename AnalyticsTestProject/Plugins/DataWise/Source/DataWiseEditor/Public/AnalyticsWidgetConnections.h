#pragma once
#include "Widgets/SUserWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/STextComboBox.h" 
#include "Widgets/SBoxPanel.h"

typedef TSharedPtr<FString> FComboItem;

class SWidgetAnalyticsConnections : public SCompoundWidget
{
	SLATE_USER_ARGS(SWidgetAnalyticsConnections)
	{ }

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void OnAddTypeChanged(FComboItem NewValue, ESelectInfo::Type);
	void UpdateEntryHeaders();
	FReply CreateNewEntry();
	void ShowEntryData();


	TArray<FComboItem> manager_types;
	FComboItem current_type;

	TSharedPtr<SVerticalBox> entry_headers;
	int selected_entry = -1;

	TSharedPtr<STextComboBox> type_selector;
	TSharedPtr<SButton> type_creator;

	TSharedPtr<SVerticalBox> content_container;
};
