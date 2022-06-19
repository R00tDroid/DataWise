#include "AnalyticsWidgetCompilation.h"
#include "DataWiseEditor.h"
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
#include "ClassFinder.h"
#include "AnalyticsCompilationStage.h"
#include "AnalyticsCompiler.h"
#include "EditorStyle.h"
#include "AssetRegistryModule.h"
#include "IAssetRegistry.h"

void SWidgetAnalyticsCompilation::Construct(const FArguments& InArgs)
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
					.OnClicked(FOnClicked::CreateSP(this, &SWidgetAnalyticsCompilation::RequestCompile))
					.IsEnabled_Lambda([this]() {return UAnalyticsCaptureManagementTools::GetSelectedInfos().Num() > 0 && UAnalyticsCaptureManagementTools::GetSelectedCompilationStages().Num() > 0; })
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
						.Text(FText::FromString("Compile"))
					]
				]
			]
		]
	];

	UpdateClassList();
	UpdateStatusDisplay();
}

void SWidgetAnalyticsCompilation::Start()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	if (AssetRegistry.IsLoadingAssets())
	{
		AssetRegistry.OnFilesLoaded().AddSP(this, &SWidgetAnalyticsCompilation::BindToAssetRegistry);
	}
	else
	{
		BindToAssetRegistry();
	}

	UAnalyticsCaptureManagementTools::OnSelectionChanged.AddSP(this, &SWidgetAnalyticsCompilation::UpdateStatusDisplay);
}

void SWidgetAnalyticsCompilation::UpdateStatusDisplay()
{
	status_display->SetText(FText::FromString(FString::FromInt(UAnalyticsCaptureManagementTools::GetSelectedInfos().Num()) + " Sessions\n" + FString::FromInt(UAnalyticsCaptureManagementTools::GetSelectedCompilationStages().Num()) +" Stages"));
}

void SWidgetAnalyticsCompilation::UpdateClassList()
{
	classes = UClassFinder::FindSubclasses(UAnalyticsCompilationStage::StaticClass());
	TArray<UClass*> class_selection = UAnalyticsCaptureManagementTools::GetSelectedCompilationStages();

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
						TArray<UClass*> selection = UAnalyticsCaptureManagementTools::GetSelectedCompilationStages();

						if(state == ECheckBoxState::Checked)
						{
							selection.AddUnique(classes[index]);
						}
						else
						{
							selection.Remove(classes[index]);
						}

						UAnalyticsCaptureManagementTools::SetSelectedCompilationStages(selection);
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

FReply SWidgetAnalyticsCompilation::RequestCompile()
{
	if (UAnalyticsCaptureManagementTools::GetSelectedInfos().Num() == 0 || UAnalyticsCaptureManagementTools::GetSelectedCompilationStages().Num() == 0) return FReply::Handled();

	UAnalyticsCacheTask* cache_task = UAnalyticsWorker::Get()->CreateTask<UAnalyticsCacheTask>();
	cache_task->captures_to_cache = UAnalyticsCaptureManagementTools::GetSelectedInfos();
	UAnalyticsWorker::Get()->AddTask(cache_task);

	UAnalyticsCompilationTask* compile_task = UAnalyticsWorker::Get()->CreateTask<UAnalyticsCompilationTask>();
	compile_task->captures_to_compile = UAnalyticsCaptureManagementTools::GetSelectedInfos();
	compile_task->stages_to_run = UAnalyticsCaptureManagementTools::GetSelectedCompilationStages();
	UAnalyticsWorker::Get()->AddTask(compile_task);

	return FReply::Handled();
}

void SWidgetAnalyticsCompilation::BindToAssetRegistry()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	AssetRegistry.OnAssetAdded().AddSP(this, &SWidgetAnalyticsCompilation::OnAssetUpdate);
	AssetRegistry.OnAssetRemoved().AddSP(this, &SWidgetAnalyticsCompilation::OnAssetUpdate);
	AssetRegistry.OnAssetRenamed().AddSP(this, &SWidgetAnalyticsCompilation::OnAssetUpdate);
}

void SWidgetAnalyticsCompilation::OnAssetUpdate(const FAssetData &)
{
	UpdateClassList();
}

void SWidgetAnalyticsCompilation::OnAssetUpdate(const FAssetData &, const FString &)
{
	UpdateClassList();
}
