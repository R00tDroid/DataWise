#include "DataWiseEditor.h"
#include "Modules/ModuleManager.h"
#include "AnalyticsWidgetConnections.h"
#include "AnalyticsWidgetSessions.h"
#include "AnalyticsWidgetCompilation.h"
#include "AnalyticsWidgetVisualization.h"
#include "EngineUtils.h"
#include "AnalyticsCompiler.h"
#include "LevelEditor.h"
#include "EditorStyle.h"
#include "MainFrame/Private/Frame/MainFrameActions.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Widgets/Docking/SDockTab.h"
#include "Editor.h"
#include "AnalyticsLocalCaptureManager.h"
#include "AnalyticsVisualizer.h"
#include "AnalyticsWorker.h"

DEFINE_LOG_CATEGORY(AnalyticsLogEditor);

IMPLEMENT_MODULE(DataWiseEditorModule, DataWiseEditor)

void DataWiseEditorModule::StartupModule()
{
	AnalyticsActionCommands::Register();
	AnalyticsActionCommands::BindCommands();

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.OnTabManagerChanged().AddLambda([]()
	{
		
		DataWiseEditorModule::LoadConfig();

		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		TSharedPtr<FTabManager> tab_manager = LevelEditorModule.GetLevelEditorTabManager();

		tab_manager->RegisterTabSpawner(connections_tab, FOnSpawnTab::CreateStatic([](const FSpawnTabArgs&)
		{
			TSharedRef<SDockTab> tab = SNew(SDockTab);
			TSharedRef<SWidget> content = SNew(SWidgetAnalyticsConnections);
			tab->SetContent(content);
			return tab;
		})).SetDisplayName(NSLOCTEXT("AnalyticsConnections", "TabTitle", "Session sources")).SetAutoGenerateMenuEntry(false);


		tab_manager->RegisterTabSpawner(session_tab, FOnSpawnTab::CreateStatic([](const FSpawnTabArgs&)
		{
			TSharedRef<SDockTab> tab = SNew(SDockTab);
			TSharedRef<SWidget> content = SNew(SWidgetAnalyticsSessions);
			tab->SetContent(content);
			return tab;
		})).SetDisplayName(NSLOCTEXT("AnalyticsSessions", "TabTitle", "Session selection")).SetAutoGenerateMenuEntry(false);


		tab_manager->RegisterTabSpawner(compilation_tab, FOnSpawnTab::CreateStatic([](const FSpawnTabArgs&)
		{
			TSharedRef<SDockTab> tab = SNew(SDockTab);
			TSharedRef<SWidget> content = SNew(SWidgetAnalyticsCompilation);
			tab->SetContent(content);
			return tab;
		})).SetDisplayName(NSLOCTEXT("AnalyticsCompilation", "TabTitle", "Compilation")).SetAutoGenerateMenuEntry(false);

		tab_manager->RegisterTabSpawner(visualization_tab, FOnSpawnTab::CreateStatic([](const FSpawnTabArgs&)
		{
			TSharedRef<SDockTab> tab = SNew(SDockTab);
			TSharedRef<SWidget> content = SNew(SWidgetAnalyticsVisualization);
			tab->SetContent(content);
			return tab;
		})).SetDisplayName(NSLOCTEXT("AnalyticsVisualization", "TabTitle", "Visualization")).SetAutoGenerateMenuEntry(false);
	});
	
	FWorldDelegates::OnPostWorldInitialization.AddLambda([](UWorld* world, UWorld::InitializationValues values)
	{
		if (world != nullptr)
		{
			if (world == GEditor->GetEditorWorldContext().World() && world->Scene != nullptr) 
			{
				UAnalyticsVisualizerComponent* visualizer = NewObject<UAnalyticsVisualizerComponent>();
				visualizer->AddToRoot();

				if (!visualizer->IsRegistered())
				{
					visualizer->SetVisibility(true);
					visualizer->RegisterComponentWithWorld(world);
				}
			}
		}
	});

	
	MainMenuExtender = MakeShareable(new FExtender);
	MainMenuExtender->AddMenuBarExtension("Window", EExtensionHook::After, AnalyticsActionCommands::menu_commands, FMenuBarExtensionDelegate::CreateStatic(&AnalyticsActionCommands::BuildMenu));
	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MainMenuExtender);
}

void DataWiseEditorModule::ShutdownModule()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<FTabManager> tab_manager = LevelEditorModule.GetLevelEditorTabManager();

	if (tab_manager.IsValid()) 
	{
		tab_manager->UnregisterTabSpawner(connections_tab);
		tab_manager->UnregisterTabSpawner(session_tab);
		tab_manager->UnregisterTabSpawner(compilation_tab);
		tab_manager->UnregisterTabSpawner(visualization_tab);
	}
}

void DataWiseEditorModule::LoadConfig()
{
	// Load config from files
	UAnalyticsCaptureManagementTools::LoadCaptureManagerConnections();
	UAnalyticsCaptureManagementTools::LoadSelectedCompilationStages();
	UAnalyticsCaptureManagementTools::LoadSessionFilters();

	// Initialize local capture manager and connection
	UAnalyticsLocalCaptureManager* manager = UAnalyticsLocalCaptureManager::GetLocalCaptureManager();
	UAnalyticsLocalCaptureManagerInfo* connection = NewObject<UAnalyticsLocalCaptureManagerInfo>();
	connection->AddToRoot();
	manager->Connection = connection;
	connection->manager = manager;

	// Load sessions without having to open the selection tab
	UAnalyticsSearchTask* task = UAnalyticsWorker::Get()->CreateTask<UAnalyticsSearchTask>();
	task->Filters = ParseFilters(UAnalyticsCaptureManagementTools::GetSessionFilters());
	task->OnSearchFinish.AddStatic(&UAnalyticsCaptureManagementTools::SetSelectedInfos);
	UAnalyticsWorker::Get()->AddTask(task);
}