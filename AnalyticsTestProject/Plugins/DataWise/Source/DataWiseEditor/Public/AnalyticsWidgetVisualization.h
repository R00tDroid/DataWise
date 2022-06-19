#pragma once
#include "ExtendedWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"


class SWidgetAnalyticsVisualization : public SExtendedWidget
{
	SLATE_USER_ARGS(SWidgetAnalyticsCompilation)
	{ }

	SLATE_END_ARGS()

	virtual void Construct(const FArguments& InArgs);
	virtual void Start() override;

	void UpdateStatusDisplay();
	void UpdateClassList();

	void UpdateSetList();

	void AddDataset();

	void BindToAssetRegistry();

	void OnAssetUpdate(const FAssetData&);
	void OnAssetUpdate(const FAssetData&, const FString&);

	TArray<UClass*> classes;
	TArray<UClass*> class_selection;

	TSharedPtr<SVerticalBox> class_list;
	TSharedPtr<SVerticalBox> dataset_list;
	TSharedPtr<STextBlock> status_display;
};
