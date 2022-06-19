#include "AnalyticsWidgetVisualization.h"
#include "AnalyticsVisualizationStage.h"
#include "DataWiseEditor.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h" 
#include "Widgets/Input/SButton.h" 
#include "Widgets/Input/SSearchBox.h" 
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Images/SThrobber.h"
#include "AnalyticsCapture.h"
#include "AnalyticsWorker.h"
#include "AnalyticsCaptureManager.h"
#include "Async/Async.h"
#include "AnalyticsLocalCaptureManager.h"
#include "ClassFinder.h"
#include "AnalyticsCompilationStage.h"
#include "AnalyticsCompiler.h"
#include "EditorStyle.h"
#include "AssetRegistryModule.h"
#include "IAssetRegistry.h"

void SWidgetAnalyticsVisualization::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					.Padding(FMargin(5.0f))
					[
						SAssignNew(class_list, SVerticalBox)
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
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(FMargin(5.0f))
					.VAlign(VAlign_Center)
					[
						SAssignNew(status_display, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin(5.0f))
					[
						SNew(SButton)
						.OnClicked(FOnClicked::CreateLambda([this]() { AddDataset(); return FReply::Handled(); }))
						.IsEnabled_Lambda([this]() {return UAnalyticsCaptureManagementTools::GetSelectedInfos().Num() > 0 && class_selection.Num() > 0; })
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
							.Text(FText::FromString("Visualize"))
						]
					]
				]
			]

			+ SVerticalBox::Slot()
			.FillHeight(.5f)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSeparator)
					.Orientation(Orient_Horizontal)
				]

				+ SVerticalBox::Slot()
				.Padding(FMargin(5.0f))
				.FillHeight(1.0f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(dataset_list, SVerticalBox)
					]
				]
			]
		]
	];

	UpdateClassList();
	UpdateStatusDisplay();
	UpdateSetList();
}

void SWidgetAnalyticsVisualization::Start()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	if (AssetRegistry.IsLoadingAssets())
	{
		AssetRegistry.OnFilesLoaded().AddSP(this, &SWidgetAnalyticsVisualization::BindToAssetRegistry);
	}
	else
	{
		BindToAssetRegistry();
	}

	UAnalyticsCaptureManagementTools::OnSelectionChanged.AddSP(this, &SWidgetAnalyticsVisualization::UpdateStatusDisplay);
	UAnalyticsVisualizerComponent::OnVisualizationDataUpdated.AddSP(this, &SWidgetAnalyticsVisualization::UpdateSetList);
}

void SWidgetAnalyticsVisualization::UpdateStatusDisplay()
{
	status_display->SetText(FText::FromString(FString::FromInt(UAnalyticsCaptureManagementTools::GetSelectedInfos().Num()) + " Sessions\n" + FString::FromInt(class_selection.Num()) + " Stages"));
}

void SWidgetAnalyticsVisualization::UpdateClassList()
{
	classes = UClassFinder::FindSubclasses(UAnalyticsVisualizationStage::StaticClass());

	int difference = classes.Num() - class_list.Get()->GetChildren()->Num();
	int pre_count = class_list.Get()->GetChildren()->Num();

	if (difference > 0)
	{
		for (int i = 0; i < std::abs(difference); i++) {
			int index = pre_count + i;

			SVerticalBox::FSlot& slot = class_list->AddSlot();
			slot
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.OnCheckStateChanged_Lambda([this, index](ECheckBoxState state)
					{
						if (state == ECheckBoxState::Checked)
						{
							class_selection.AddUnique(classes[index]);
						}
						else
						{
							class_selection.Remove(classes[index]);
						}

						UpdateClassList();
						UpdateStatusDisplay();
					})
				]

			+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(FMargin(5.0f, 0, 0, 0))
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
				]
				];

			slot.Padding(FMargin(0, 0, 0, 10));
		}
	}
	else if (difference < 0)
	{
		for (int i = 0; i < std::abs(difference); i++)
		{
			class_list->RemoveSlot(class_list.Get()->GetChildren()->GetChildAt(class_list.Get()->GetChildren()->Num() - 1));
		}
	}

	for (int i = 0; i < class_list.Get()->GetChildren()->Num(); i++)
	{
		SHorizontalBox* entry = static_cast<SHorizontalBox*>(&class_list.Get()->GetChildren()->GetChildAt(i).Get());
		TSharedRef<SCheckBox> selection_check = StaticCastSharedRef<SCheckBox>(entry->GetChildren()->GetChildAt(0));
		TSharedRef<STextBlock> name_label = StaticCastSharedRef<STextBlock>(entry->GetChildren()->GetChildAt(1));
		selection_check->SetIsChecked(class_selection.Contains(classes[i]) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
		name_label->SetText(FText::FromString(classes[i]->GetName().Replace(TEXT("_C"), TEXT(""))));
	}
}

void SWidgetAnalyticsVisualization::UpdateSetList()
{
	TArray<FAnalyticsVisualizationSet>& data_sets = UAnalyticsVisualizerComponent::GetDatasets();

	int difference = data_sets.Num() - dataset_list.Get()->GetChildren()->Num();
	int pre_count = dataset_list.Get()->GetChildren()->Num();

	if (difference > 0)
	{
		for (int i = 0; i < std::abs(difference); i++) 
		{
			int index = pre_count + i;

			SVerticalBox::FSlot& slot = dataset_list->AddSlot();

			slot.Padding(FMargin(0, 0, 0, 10))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
					.Padding(FMargin(0, 0, 10, 0))
					.AutoWidth()
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Lambda([index](ECheckBoxState state) {
							TArray<FAnalyticsVisualizationSet>& data_sets = UAnalyticsVisualizerComponent::GetDatasets();
							data_sets[index].Enabled = state == ECheckBoxState::Checked;
							UAnalyticsVisualizerComponent::NotifyDatasetUpdate();
						})
					]

				+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SVerticalBox)
						.Clipping(EWidgetClipping::ClipToBounds)
						+ SVerticalBox::Slot()
							[
								SNew(STextBlock)
								.Text(FText::FromString("0 Sessions"))
							]
						+ SVerticalBox::Slot()
							[
								SNew(STextBlock)
								.Text(FText::FromString("0 Stages"))
							]
					]

				+ SHorizontalBox::Slot()
					.Padding(FMargin(5, 0, 0, 0))
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SGridPanel)
						+ SGridPanel::Slot(0, 0)
							[
								SNew(SButton)
								.Visibility(EVisibility::Hidden)
								.OnClicked_Lambda([index]() {
									TArray<FAnalyticsVisualizationSet>& data_sets = UAnalyticsVisualizerComponent::GetDatasets();
									UAnalyticsVisualizerComponent::RemoveDataset(data_sets[index]);
									return FReply::Handled();
								})
								[
									SNew(SImage)
									.Image(FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.AssetActions.Delete").GetIcon())
								]
							]

						+ SGridPanel::Slot(0, 0)
							[
								SNew(SCircularThrobber)
							]

						+ SGridPanel::Slot(0, 0)
							[
								SNew(SProgressBar)
							]
					]
			];
		}
	}
	else if (difference < 0)
	{
		for (int i = 0; i < std::abs(difference); i++)
		{
			dataset_list->RemoveSlot(dataset_list.Get()->GetChildren()->GetChildAt(dataset_list.Get()->GetChildren()->Num() - 1));
		}
	}

	for (int i = 0; i < dataset_list.Get()->GetChildren()->Num(); i++)
	{
		SHorizontalBox* entry = static_cast<SHorizontalBox*>(&dataset_list.Get()->GetChildren()->GetChildAt(i).Get());
		TSharedRef<SCheckBox> selection_check = StaticCastSharedRef<SCheckBox>(entry->GetChildren()->GetChildAt(0));
		TSharedRef<SVerticalBox> label_container = StaticCastSharedRef<SVerticalBox>(entry->GetChildren()->GetChildAt(1));
		TSharedRef<SGridPanel> status_container = StaticCastSharedRef<SGridPanel>(entry->GetChildren()->GetChildAt(2));

		TSharedRef<STextBlock> label_sessions = StaticCastSharedRef<STextBlock>(label_container->GetChildren()->GetChildAt(0));
		TSharedRef<STextBlock> label_stages = StaticCastSharedRef<STextBlock>(label_container->GetChildren()->GetChildAt(1));
		TSharedRef<SButton> button_delete = StaticCastSharedRef<SButton>(status_container->GetChildren()->GetChildAt(0));
		TSharedRef<SCircularThrobber> throbber_compiling = StaticCastSharedRef<SCircularThrobber>(status_container->GetChildren()->GetChildAt(1));
		TSharedRef<SProgressBar> progress_compiling = StaticCastSharedRef<SProgressBar>(status_container->GetChildren()->GetChildAt(2));

		selection_check->SetIsChecked(data_sets[i].Enabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
		button_delete->SetVisibility(data_sets[i].Valid ? EVisibility::Visible : EVisibility::Hidden);
		throbber_compiling->SetVisibility((!data_sets[i].Valid && data_sets[i].Progress == 0.0f) ? EVisibility::Visible : EVisibility::Hidden);
		progress_compiling->SetVisibility((!data_sets[i].Valid && data_sets[i].Progress != 0.0f) ? EVisibility::Visible : EVisibility::Hidden);

		progress_compiling->SetPercent(data_sets[i].Progress);

		label_sessions->SetText(FText::FromString(FString::FromInt(data_sets[i].Sessions.Num()) + " Sessions: " + data_sets[i].FilterString));

		FString stage_string;
		for (UClass* stage_class : data_sets[i].Stages) {
			stage_string += stage_class->GetName().Replace(TEXT("_C"), TEXT(""));
			stage_string += ", ";
		}
		stage_string.RemoveFromEnd(", ");

		label_stages->SetText(FText::FromString(FString::FromInt(data_sets[i].Stages.Num()) + " Stages: " + stage_string));
	}
}

void SWidgetAnalyticsVisualization::AddDataset()
{
	TArray<FAnalyticsVisualizationSet>& datasets = UAnalyticsVisualizerComponent::GetDatasets();

	FAnalyticsVisualizationSet set;
	set.Sessions = UAnalyticsCaptureManagementTools::GetSelectedInfos();
	set.Stages = class_selection;
	set.FilterString = UAnalyticsCaptureManagementTools::GetSessionFilters();

	if (datasets.Find(set) == INDEX_NONE)
	{
		UAnalyticsVisualizerComponent::AddDataset(set);
	}
}

void SWidgetAnalyticsVisualization::BindToAssetRegistry()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	AssetRegistry.OnAssetAdded().AddSP(this, &SWidgetAnalyticsVisualization::OnAssetUpdate);
	AssetRegistry.OnAssetRemoved().AddSP(this, &SWidgetAnalyticsVisualization::OnAssetUpdate);
	AssetRegistry.OnAssetRenamed().AddSP(this, &SWidgetAnalyticsVisualization::OnAssetUpdate);
}

void SWidgetAnalyticsVisualization::OnAssetUpdate(const FAssetData &)
{
	UpdateClassList();
}

void SWidgetAnalyticsVisualization::OnAssetUpdate(const FAssetData & asset, const FString &)
{
	OnAssetUpdate(asset);
}
