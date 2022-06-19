#include "AnalyticsCaptureManager.h"
#include "DataWise.h"
#include "AnalyticsLocalCaptureManager.h"
#include "Engine/Engine.h"
#include "ASync/Async.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "ClassFinder.h"
#include "HAL/PlatformFilemanager.h"


UAnalyticsCaptureManagementTools::FOnSelectionChanged UAnalyticsCaptureManagementTools::OnSelectionChanged;
UAnalyticsCaptureManagementTools::FOnConnectionsChanged UAnalyticsCaptureManagementTools::OnConnectionsChanged;


void UAnalyticsCaptureManagementTools::TransferCapture(FAnalyticsCaptureInfo info, UAnalyticsCaptureManager* source, UAnalyticsCaptureManager* destination)
{
	if (source == nullptr || destination == nullptr)
	{
		UE_LOG(AnalyticsLog, Error, TEXT("Could not transfer capture due to invalid source or desination"));
		return;
	}

	UAnalyticsCapture* capture = source->DeserializeCapture(info);
	if(capture != nullptr)
	{
		FAnalyticsCaptureInfo result = destination->SerializeCapture(capture);
		if(!result.Name.IsEmpty()) source->DeleteStoredCapture(info);
	}
}

void UAnalyticsCaptureManagementTools::TransferCapture_Latent(UObject* WorldContextObject, FLatentActionInfo LatentInfo, FAnalyticsCaptureInfo info, UAnalyticsCaptureManager* source, UAnalyticsCaptureManager* destination)
{
	if (UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FTransferCaptureAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == NULL)
		{
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FTransferCaptureAction(info, source, destination, LatentInfo));
		}
	}
}

FTransferCaptureAction::FTransferCaptureAction(FAnalyticsCaptureInfo Info, UAnalyticsCaptureManager* Source, UAnalyticsCaptureManager* Destination, const FLatentActionInfo& LatentInfo) : ExecutionFunction(LatentInfo.ExecutionFunction), OutputLink(LatentInfo.Linkage), CallbackTarget(LatentInfo.CallbackTarget)
{
	finish_flag = false;

	FFunctionGraphTask::CreateAndDispatchWhenReady([Info, Source, Destination, this]()
	{
		UAnalyticsCaptureManagementTools::TransferCapture(Info, Source, Destination);
		this->finish_flag = true;
	}, TStatId(), nullptr, ENamedThreads::AnyNormalThreadNormalTask);
}
	
void FTransferCaptureAction::UpdateOperation(FLatentResponse & Response)
{
	Response.FinishAndTriggerIf(finish_flag, ExecutionFunction, OutputLink, CallbackTarget);
}



void UAnalyticsCaptureManagementTools::TransferAllCaptures(UAnalyticsCaptureManager* source, UAnalyticsCaptureManager* destination)
{
	if(source == nullptr || destination == nullptr)
	{
		UE_LOG(AnalyticsLog, Error, TEXT("Could not transfer captures due to invalid source or desination"));
		return;
	}

	TArray<FAnalyticsCaptureInfo> captures = source->FindCaptures();
	for(FAnalyticsCaptureInfo& capture : captures)
	{
		TransferCapture(capture, source, destination);
	}
}

void UAnalyticsCaptureManagementTools::TransferAllCaptures_Latent(UObject* WorldContextObject, FLatentActionInfo LatentInfo, UAnalyticsCaptureManager* source, UAnalyticsCaptureManager* destination)
{
	if (UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FTransferAllCapturesAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == NULL)
		{
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FTransferAllCapturesAction(source, destination, LatentInfo));
		}
	}
}

FTransferAllCapturesAction::FTransferAllCapturesAction(UAnalyticsCaptureManager* Source, UAnalyticsCaptureManager* Destination, const FLatentActionInfo & LatentInfo) : ExecutionFunction(LatentInfo.ExecutionFunction), OutputLink(LatentInfo.Linkage), CallbackTarget(LatentInfo.CallbackTarget)
{
	finish_flag = false;

	FFunctionGraphTask::CreateAndDispatchWhenReady([Source, Destination, this]()
	{
		UAnalyticsCaptureManagementTools::TransferAllCaptures(Source, Destination);
		this->finish_flag = true;
	}, TStatId(), nullptr, ENamedThreads::AnyNormalThreadNormalTask);
}

void FTransferAllCapturesAction::UpdateOperation(FLatentResponse& Response)
{
	Response.FinishAndTriggerIf(finish_flag, ExecutionFunction, OutputLink, CallbackTarget);
}


void UAnalyticsCaptureManagementTools::CacheCapture(FAnalyticsCaptureInfo info, UAnalyticsCaptureManager* source)
{
	UAnalyticsCapture* capture = source->DeserializeCapture(info);
	if (!capture->Name.IsEmpty())
	{
		UAnalyticsLocalCaptureManager::GetLocalCaptureManager()->SerializeCaptureToCache(capture);
	}
}




TArray<UAnalyticsCaptureManagerConnection*> registered_capture_manager_connections;
TMap<FString, UClass*> registered_capture_manager_connection_tags;

void UAnalyticsCaptureManagementTools::LoadCaptureManagerConnections()
{
	TArray<UClass*> class_types = UClassFinder::FindSubclasses(UAnalyticsCaptureManagerConnection::StaticClass());

	FString path = FPaths::ProjectSavedDir() + "Analytics/session_sources.bin";
	FArchive* archive = IFileManager::Get().CreateFileReader(*path);

	if (archive == nullptr) { return; }
	if (archive->IsError()) { archive->Close(); return; }

	while (archive->Tell() < archive->TotalSize() - 1)
	{
		FString class_type;
		*archive << class_type;

		if (class_type.IsEmpty()) { archive->Close(); return; }

		UClass* found_class = nullptr;

		for (UClass* possible_class : class_types)
		{
			if (possible_class->GetName().Equals(class_type)) { found_class = possible_class; break; }
		}

		if (found_class == nullptr)
		{
			UE_LOG(AnalyticsLog, Error, TEXT("No matching class found! name: %s"), *class_type);
			archive->Close();
			return;
		}

		UAnalyticsCaptureManagerConnection* info = NewObject<UAnalyticsCaptureManagerConnection>((UObject*)GetTransientPackage(), found_class);
		info->AddToRoot();
		registered_capture_manager_connections.Add(info);

		for (TFieldIterator<UProperty> property_iterator(found_class); property_iterator; ++property_iterator)
		{
			UProperty* prop = *property_iterator;
			FString type = prop->GetCPPType();
			FString name = prop->GetNameCPP();

			if (type.Equals("FString")) {
				FString* data = prop->ContainerPtrToValuePtr<FString>(info);
				*archive << *data;
			}
			else if (type.Equals("int32")) {
				int32* data = prop->ContainerPtrToValuePtr<int32>(info);
				*archive << *data;
			}
			
			else if (type.Equals("bool")) {
				bool* data = prop->ContainerPtrToValuePtr<bool>(info);
				*archive << *data;
			}
			else
			{
				UE_LOG(AnalyticsLog, Error, TEXT("Unsupported property type: %s (%s)"), *type, *name); break;
			}
		}
	}

	archive->Close();

	OnConnectionsChanged.Broadcast();
}

void UAnalyticsCaptureManagementTools::SaveCaptureManagerConnections()
{
	FString path = FPaths::ProjectSavedDir() + "Analytics/session_sources.bin";

	if(FPaths::FileExists(path))
	{
		FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*path);
	}

	FArchive* archive = IFileManager::Get().CreateFileWriter(*path, FILEWRITE_EvenIfReadOnly);

	if(archive==nullptr)
	{
		UE_LOG(AnalyticsLog, Error, TEXT("Could not save session source list"));
		return;
	}

	for(UAnalyticsCaptureManagerConnection* info : registered_capture_manager_connections)
	{
		FString class_name = info->GetClass()->GetName();
		*archive << class_name;

		for (TFieldIterator<UProperty> property_iterator(info->GetClass()); property_iterator; ++property_iterator)
		{
			UProperty* prop = *property_iterator;
			FString type = prop->GetCPPType();
			FString name = prop->GetNameCPP();

			if (type.Equals("FString")) {
				FString* data = prop->ContainerPtrToValuePtr<FString>(info);
				*archive << *data;
			}
			else if (type.Equals("int32")) {
				int32* data = prop->ContainerPtrToValuePtr<int32>(info);
				*archive << *data;
			}
			else if (type.Equals("bool")) {
				bool* data = prop->ContainerPtrToValuePtr<bool>(info);
				*archive << *data;
			}
			else
			{
				UE_LOG(AnalyticsLog, Error, TEXT("Unsupported property type: %s (%s)"), *type, *name);
				break;
			}
		}
	}

	archive->Flush();
	archive->Close();
}

TArray<UAnalyticsCaptureManagerConnection*> UAnalyticsCaptureManagementTools::GetCaptureManagerConnections()
{
	return registered_capture_manager_connections;
}

void UAnalyticsCaptureManagementTools::RegisterCaptureManagerConnection(UAnalyticsCaptureManagerConnection* info)
{
	registered_capture_manager_connections.Add(info);
	SaveCaptureManagerConnections();
	OnConnectionsChanged.Broadcast();
}

void UAnalyticsCaptureManagementTools::UnregisterCaptureManagerConnection(UAnalyticsCaptureManagerConnection* info)
{
	registered_capture_manager_connections.Remove(info);
	SaveCaptureManagerConnections();
	OnConnectionsChanged.Broadcast();
}

TMap<FString, UClass*> UAnalyticsCaptureManagementTools::GetCaptureManagerConnectionTags()
{
	return registered_capture_manager_connection_tags;
}



TArray<FAnalyticsCaptureInfo> selected_infos;

void UAnalyticsCaptureManagementTools::SetSelectedInfos(TArray<FAnalyticsCaptureInfo> infos)
{
	selected_infos = infos;
	OnSelectionChanged.Broadcast();
}

TArray<FAnalyticsCaptureInfo> UAnalyticsCaptureManagementTools::GetSelectedInfos()
{
	return selected_infos;
}






TArray<UClass*> selected_stages;

void UAnalyticsCaptureManagementTools::LoadSelectedCompilationStages()
{
	FString path = FPaths::ProjectSavedDir() + "Analytics/compilation_stages.bin";
	FArchive* archive = IFileManager::Get().CreateFileReader(*path);

	if (archive == nullptr) { return; }
	if (archive->IsError()) { archive->Close(); return; }

	TArray<UClass*> class_types = UClassFinder::FindSubclasses(UObject::StaticClass());

	while (archive->Tell() < archive->TotalSize() - 1)
	{
		FString class_type;
		*archive << class_type;

		if (class_type.IsEmpty()) { archive->Close(); return; }

		UClass* found_class = nullptr;

		for (UClass* possible_class : class_types)
		{
			if (possible_class->GetName().Equals(class_type)) { found_class = possible_class; break; }
		}

		if (found_class == nullptr)
		{
			UE_LOG(AnalyticsLog, Error, TEXT("No matching class found! name: %s"), *class_type);
			archive->Close();
			return;
		}

		selected_stages.Add(found_class);
	}

	archive->Close();
}

void UAnalyticsCaptureManagementTools::SaveSelectedCompilationStages()
{
	FString path = FPaths::ProjectSavedDir() + "Analytics/compilation_stages.bin";

	if (FPaths::FileExists(path))
	{
		FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*path);
	}

	FArchive* archive = IFileManager::Get().CreateFileWriter(*path, FILEWRITE_EvenIfReadOnly);

	if (archive == nullptr)
	{
		UE_LOG(AnalyticsLog, Error, TEXT("Could not save stage list"));
		return;
	}

	for (UClass* stage : selected_stages)
	{
		FString class_name = stage->GetName();
		*archive << class_name;
	}

	archive->Flush();
	archive->Close();
}

void UAnalyticsCaptureManagementTools::SetSelectedCompilationStages(TArray<UClass*> stages)
{
	selected_stages = stages;
	SaveSelectedCompilationStages();
}

TArray<UClass*> UAnalyticsCaptureManagementTools::GetSelectedCompilationStages()
{
	return selected_stages;
}

CaptureManagerTagRegistration::CaptureManagerTagRegistration(UClass* type, FString tag)
{
	registered_capture_manager_connection_tags.Add(tag, type);
}





FString filter_string;

void UAnalyticsCaptureManagementTools::LoadSessionFilters()
{
	FString path = FPaths::ProjectSavedDir() + "Analytics/filters.bin";
	FArchive* archive = IFileManager::Get().CreateFileReader(*path);

	if (archive == nullptr) { return; }
	if (archive->IsError()) { archive->Close(); return; }

	*archive << filter_string;

	archive->Close();
}

void UAnalyticsCaptureManagementTools::SaveSessionFilters()
{
	FString path = FPaths::ProjectSavedDir() + "Analytics/filters.bin";

	if (FPaths::FileExists(path))
	{
		FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*path);
	}

	FArchive* archive = IFileManager::Get().CreateFileWriter(*path, FILEWRITE_EvenIfReadOnly);

	if (archive == nullptr)
	{
		UE_LOG(AnalyticsLog, Error, TEXT("Could not save filter list"));
		return;
	}

	*archive << filter_string;
	
	archive->Flush();
	archive->Close();
}

void UAnalyticsCaptureManagementTools::SetSessionFilters(FString data)
{
	filter_string = data;
	SaveSessionFilters();
}

FString UAnalyticsCaptureManagementTools::GetSessionFilters()
{
	return filter_string;
}


