#include "AnalyticsCompiler.h"
#include "DataWiseEditor.h"
#include "AnalyticsCompilationStage.h"
#include "AnalyticsVisualizationStage.h"
#include "Kismet2/DebuggerCommands.h"
#include "EditorStyle.h"
#include "ClassFinder.h"
#include "AnalyticsLocalCaptureManager.h"
#include "AnalyticsWorker.h"
#include "Framework/Commands/Commands.h"
#include "Engine/Engine.h"
#include "LevelEditor.h"

bool UAnalyticsCacheTask::Execute()
{
	if(notify) ShowNotification("Checking analytics data cache", SNotificationItem::CS_Pending, false);
	UAnalyticsLocalCaptureManager* local_manager = UAnalyticsLocalCaptureManager::GetLocalCaptureManager();
	TArray<FAnalyticsCaptureInfo> cached_captures = local_manager->FindCachedCaptures();
	TArray<FAnalyticsCaptureInfo> to_cache;

	uint32 cached = 1;
	uint32 total_cache = captures_to_cache.Num();
	for(FAnalyticsCaptureInfo capture : captures_to_cache)
	{
		if (notify) SetNotificationText("Checking analytics data cache " + FString::FromInt(cached) + " / " + FString::FromInt(total_cache));
		if(!cached_captures.Contains(capture))
		{
			to_cache.Add(capture);
		}
		cached++;
	}

	SetNotificationState(SNotificationItem::CS_Success);
	DismissNotification();

	if (to_cache.Num() != 0) {

		if (notify) ShowNotification("Caching analytics data", SNotificationItem::CS_Pending, false);

		cached = 1;
		total_cache = to_cache.Num();
		for (FAnalyticsCaptureInfo capture : to_cache)
		{
			if (notify) SetNotificationText("Caching analytics data " + FString::FromInt(cached) + " / " + FString::FromInt(total_cache));
			if (capture.Source.IsValid()) 
			{
				UAnalyticsCaptureManagementTools::CacheCapture(capture, capture.Source->GetManager());
			} else
			{
				UE_LOG(AnalyticsLogEditor, Warning, TEXT("Could not cache: %s"), *capture.Name);
			}
			cached++;
		}

		SetNotificationState(SNotificationItem::CS_Success);
		DismissNotification();
	}

	if (notify) ShowNotification("Finished caching analytics data");

	return true;
}

bool UAnalyticsCompilationTask::Execute()
{
	ShowNotification("Preparing analytics data compilation", SNotificationItem::CS_Pending, false);

	TArray<UClass*> stage_types = UAnalyticsCaptureManagementTools::GetSelectedCompilationStages();

	TArray<UAnalyticsCapture*> captures;
	TArray<UAnalyticsCompilationStage*> stages;

	uint32 captures_loaded = 1;
	uint32 capture_count = captures_to_compile.Num();
	for (FAnalyticsCaptureInfo capture_index : captures_to_compile)
	{
		UAnalyticsCapture* cap = UAnalyticsLocalCaptureManager::GetLocalCaptureManager()->DeserializeCapture(capture_index);
		if (cap != nullptr) captures.Add(cap);
		SetNotificationText("Preparing analytics data compilation " + FString::FromInt(captures_loaded) + " / " + FString::FromInt(capture_count));
		captures_loaded++;
	}

	for (UClass* stage_type : stage_types)
	{
		UAnalyticsCompilationStage* stage = NewObject<UAnalyticsCompilationStage>(GetTransientPackage(), stage_type);
		stage->AddToRoot();
		stages.Add(stage);
	}

	uint32 task_count = stages.Num() * captures.Num() + stages.Num();
	uint32 task_index = 1;

	SetNotificationState(SNotificationItem::CS_Success);
	DismissNotification();
	ShowNotification("Compiling analytics data", SNotificationItem::CS_Pending, false);

	UAnalyticsCompilerContext* context = NewObject<UAnalyticsCompilerContext>();
	context->AddToRoot();

	for (UAnalyticsCompilationStage* stage : stages)
	{
		UE_LOG(AnalyticsLogEditor, Log, TEXT("Executing stage: %s"), *stage->GetClass()->GetName());

		for (UAnalyticsCapture* capture : captures)
		{
			SetNotificationText("Compiling analytics data " + FString::FromInt(task_index) + " / " + FString::FromInt(task_count));

			context->Name = capture->Name;
			context->Packets = capture->packets;
			stage->PreProcessCapture();
			stage->ProcessCapture(context);
			stage->PostProcessCapture();
			stage->ExportData(capture->Name);

			task_index++;
		}

		SetNotificationText("Compiling analytics data " + FString::FromInt(task_index) + " / " + FString::FromInt(task_count));
		context->Name = "";
		context->Packets.Empty();
		for (UAnalyticsCapture* capture : captures)
		{
			for (UAnalyticsPacket* packet : capture->packets)
			{
				context->Packets.Add(packet);
			}
		}
		stage->PreProcessCaptureGroup();
		stage->ProcessCaptureGroup(context);
		stage->PostProcessCaptureGroup();
		stage->ExportData("");

		task_index++;
	}

	SetNotificationState(SNotificationItem::CS_Success);
	DismissNotification();

	ShowNotification("Finished compiling analytics data", SNotificationItem::CS_None, true, 5.0f);

	SetNotificationLink("Output directory", FSimpleDelegate::CreateLambda([this]() {
		FString output_directory = FPaths::ProjectDir() + local_data_output;
		FPlatformProcess::ExploreFolder(*output_directory);
	}));

	context->RemoveFromRoot();
	context->ConditionalBeginDestroy();

	for (UAnalyticsCapture* capture : captures)
	{
		capture->ConditionalBeginDestroy();
	}

	for (UAnalyticsCompilationStage* stage : stages)
	{
		
		stage->RemoveFromRoot();
		stage->ConditionalBeginDestroy();
		stage->MarkPendingKill();
	}
	stages.Empty();

	GEngine->ForceGarbageCollection(true);

	return true;
}

bool UAnalyticsVisualizationTask::Execute() 
{
	if (notify) ShowNotification("Visualizing data", SNotificationItem::CS_Pending, true, 2.0f);
	DismissNotification();

	FAnalyticsVisualizationSet dataset = dataset_to_compile;

	TArray<UAnalyticsCapture*> captures;
	TArray<UAnalyticsVisualizationStage*> stages;

	uint32 captures_loaded = 1;
	uint32 capture_count = dataset.Sessions.Num();

	uint32 index = 0;

	for (FAnalyticsCaptureInfo capture_index : dataset.Sessions)
	{
		UAnalyticsCapture* cap = UAnalyticsLocalCaptureManager::GetLocalCaptureManager()->DeserializeCapture(capture_index);
		if (cap != nullptr) captures.Add(cap);
		if (notify) SetNotificationText("Preparing analytics data visualization " + FString::FromInt(captures_loaded) + " / " + FString::FromInt(capture_count));
		captures_loaded++;

		dataset.Progress = (1.0f / 3) * index / (float)capture_count;
		NotifyUpdate(dataset);
		index++;
	}

	index = 0;
	for (UClass* stage_type : dataset.Stages)
	{
		UAnalyticsVisualizationStage* stage = NewObject<UAnalyticsVisualizationStage>(GetTransientPackage(), stage_type);
		stage->AddToRoot();
		stages.Add(stage);

		dataset.Progress = (1.0f / 3) + (1.0f / 3) * index / (float)dataset.Stages.Num();
		NotifyUpdate(dataset);
		index++;
	}

	uint32 task_count = stages.Num() * captures.Num() + stages.Num();
	uint32 task_index = 1;

	SetNotificationState(SNotificationItem::CS_Success);
	DismissNotification();
	if (notify) ShowNotification("Visualizing analytics data", SNotificationItem::CS_Pending, false);

	UAnalyticsCompilerContext* context = NewObject<UAnalyticsCompilerContext>();
	context->AddToRoot();

	for (UAnalyticsVisualizationStage* stage : stages)
	{
		UE_LOG(AnalyticsLogEditor, Log, TEXT("Executing stage: %s"), *stage->GetClass()->GetName());

		for (UAnalyticsCapture* capture : captures)
		{
			if (notify) SetNotificationText("Visualizing analytics data " + FString::FromInt(task_index) + " / " + FString::FromInt(task_count));

			context->Name = capture->Name;
			context->Packets = capture->packets;
			stage->PreProcessCapture();
			stage->ProcessCapture(context);
			stage->PostProcessCapture();
			stage->ExportData(dataset.Output);

			task_index++;

			dataset.Progress = (1.0f / 3) * 2 + (1.0f / 3) * task_index / (float)task_count;
			NotifyUpdate(dataset);
		}

		if (notify) SetNotificationText("Visualizing analytics data " + FString::FromInt(task_index) + " / " + FString::FromInt(task_count));
		context->Name = "Merged";
		context->Packets.Empty();
		for (UAnalyticsCapture* capture : captures)
		{
			for (UAnalyticsPacket* packet : capture->packets)
			{
				context->Packets.Add(packet);
			}
		}
		stage->PreProcessCaptureGroup();
		stage->ProcessCaptureGroup(context);
		stage->PostProcessCaptureGroup();
		stage->ExportData(dataset.Output);

		task_index++;

		dataset.Progress = (1.0f / 3) * 2 + (1.0f / 3) * task_index / (float)task_count;
		NotifyUpdate(dataset);
	}

	if (notify) SetNotificationState(SNotificationItem::CS_Success);
	if (notify) DismissNotification();

	context->RemoveFromRoot();
	context->ConditionalBeginDestroy();

	for (UAnalyticsCapture* capture : captures)
	{
		capture->ConditionalBeginDestroy();
	}

	for (UAnalyticsVisualizationStage* stage : stages)
	{

		stage->RemoveFromRoot();
		stage->ConditionalBeginDestroy();
		stage->MarkPendingKill();
	}
	stages.Empty();

	dataset.Valid = true;
	dataset.Progress = 1;
	NotifyUpdate(dataset);

	return true;
}

void UAnalyticsVisualizationTask::NotifyUpdate(FAnalyticsVisualizationSet& UpdatedSet)
{
	AsyncTask(ENamedThreads::GameThread, [UpdatedSet]()
	{
		TArray<FAnalyticsVisualizationSet>& datasets = UAnalyticsVisualizerComponent::GetDatasets();

		int index;
		if (datasets.Find(UpdatedSet, index)) {
			datasets[index].Progress = UpdatedSet.Progress;
			datasets[index].Valid = UpdatedSet.Valid;
			datasets[index].Output = UpdatedSet.Output;
			UAnalyticsVisualizerComponent::NotifyDatasetUpdate();
		}
		else {
			UE_LOG(AnalyticsLogEditor, Error, TEXT("Dataset not found!"));
		}
	});
}

#define LOCTEXT_NAMESPACE "DataWiseEditorModule"

TSharedPtr<FUICommandList> AnalyticsActionCommands::menu_commands;


AnalyticsActionCommands::AnalyticsActionCommands() : TCommands<AnalyticsActionCommands>("CompileBtn", LOCTEXT("CompileBtnDesc", ""), "MainFrame", FEditorStyle::GetStyleSetName())
{

}

class AnalyticsActionCommandsCallbacks : public FPlayWorldCommandCallbacks
{
public:
	static bool CompileBtn_CanExecute() { return !HasPlayWorld(); }
	static bool CompileBtn_CanShow() { return !HasPlayWorld(); }
};


void AnalyticsActionCommands::RegisterCommands()
{

	UI_COMMAND(open_connections, "Connections", "", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(open_sessionselection, "Session selection", "", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(open_compilation, "Compilation", "", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(open_visualization, "Visualization", "", EUserInterfaceActionType::Button, FInputChord());
}


void AnalyticsActionCommands::BindCommands()
{
	menu_commands = MakeShareable(new FUICommandList);
	const AnalyticsActionCommands& Commands = AnalyticsActionCommands::Get();

	FUICommandList& menu_actions = *menu_commands;
	menu_actions.MapAction(Commands.open_connections, FExecuteAction::CreateLambda([](){ FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor").GetLevelEditorTabManager()->InvokeTab(connections_tab); }));
	menu_actions.MapAction(Commands.open_sessionselection, FExecuteAction::CreateLambda([]() { FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor").GetLevelEditorTabManager()->InvokeTab(session_tab); }));
	menu_actions.MapAction(Commands.open_compilation, FExecuteAction::CreateLambda([]() { FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor").GetLevelEditorTabManager()->InvokeTab(compilation_tab); }));
	menu_actions.MapAction(Commands.open_visualization, FExecuteAction::CreateLambda([]() { FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor").GetLevelEditorTabManager()->InvokeTab(visualization_tab); }));
}

void AnalyticsActionCommands::BuildMenu(FMenuBarBuilder& MenuBuilder)
{
	MenuBuilder.AddPullDownMenu(LOCTEXT("", "Analytics"), LOCTEXT("", "Open analytics tools"), FNewMenuDelegate::CreateStatic(&AnalyticsActionCommands::MakeMenu), "Analytics");
}

void AnalyticsActionCommands::MakeMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(AnalyticsActionCommands::Get().open_connections);
	MenuBuilder.AddMenuEntry(AnalyticsActionCommands::Get().open_sessionselection);
	MenuBuilder.AddMenuEntry(AnalyticsActionCommands::Get().open_compilation);
	MenuBuilder.AddMenuEntry(AnalyticsActionCommands::Get().open_visualization);
}

#undef LOCTEXT_NAMESPACE
