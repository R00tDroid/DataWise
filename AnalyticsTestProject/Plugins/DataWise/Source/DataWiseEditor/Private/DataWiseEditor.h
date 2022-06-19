#pragma once

#include "Modules/ModuleInterface.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Framework/Docking/TabManager.h"


DECLARE_LOG_CATEGORY_EXTERN(AnalyticsLogEditor, Log, All);

class DataWiseEditorModule : public IModuleInterface
{
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static void LoadConfig();

private:
	TSharedPtr<FExtender> MainMenuExtender;
};