#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AnalyticsWorkerTask.h"
#include "AnalyticsCaptureManager.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "AnalyticsVisualizer.h"
#include "AnalyticsCompiler.generated.h"

const FName connections_tab = FName(TEXT("AnalyticsConnectionsTab"));
const FName session_tab = FName(TEXT("AnalyticsSessionTab"));
const FName compilation_tab = FName(TEXT("AnalyticsCompilationTab"));
const FName visualization_tab = FName(TEXT("AnalyticsVisualizationTab"));

UCLASS()
class UAnalyticsCompilationTask : public UAnalyticsWorkerTask {
	GENERATED_BODY()
public:
	bool Execute() override;

	TArray<FAnalyticsCaptureInfo> captures_to_compile;
	TArray<UClass*> stages_to_run;
};

UCLASS()
class UAnalyticsVisualizationTask : public UAnalyticsWorkerTask {
	GENERATED_BODY()
public:
	bool Execute() override;

	FAnalyticsVisualizationSet dataset_to_compile;
	bool notify = false;

	void NotifyUpdate(FAnalyticsVisualizationSet& UpdatedSet);
};

UCLASS()
class UAnalyticsCacheTask : public UAnalyticsWorkerTask {
	GENERATED_BODY()
public:
	bool Execute() override;

	TArray<FAnalyticsCaptureInfo> captures_to_cache;
	bool notify = true;
};


class AnalyticsActionCommands : public TCommands<AnalyticsActionCommands>
{
private:

	friend class TCommands<AnalyticsActionCommands>;

	AnalyticsActionCommands();

public:

	virtual void RegisterCommands() override;

	static void BindCommands();

	static void BuildMenu(FMenuBarBuilder& MenuBuilder);
	static void MakeMenu(FMenuBuilder& MenuBuilder);

	static TSharedPtr<FUICommandList> menu_commands;

private:
	TSharedPtr<FUICommandInfo> open_connections;
	TSharedPtr<FUICommandInfo> open_sessionselection;
	TSharedPtr<FUICommandInfo> open_compilation;
	TSharedPtr<FUICommandInfo> open_visualization;
};