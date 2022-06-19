#pragma once
#include "ExtendedWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"


class SWidgetAnalyticsCompilation : public SExtendedWidget
{
	SLATE_USER_ARGS(SWidgetAnalyticsCompilation)
	{ }

	SLATE_END_ARGS()

	virtual void Construct(const FArguments& InArgs);
	virtual void Start() override;

	void UpdateStatusDisplay();
	void UpdateClassList();

	FReply RequestCompile();
	void BindToAssetRegistry();

	void OnAssetUpdate(const FAssetData&);
	void OnAssetUpdate(const FAssetData&, const FString&);

	TArray<UClass*> classes;
	TSharedPtr<SVerticalBox> class_list;
	TSharedPtr<STextBlock> status_display;
};
