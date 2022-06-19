#include "AnalyticsFTPCaptureManager.h"
#include "DataWiseFTP.h"
#include "AnalyticsLocalCaptureManager.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"

FAnalyticsCaptureInfo UAnalyticsFTPCaptureManager::SerializeCapture(UAnalyticsCapture* capture)
{
	TArray<uint8> buffer;
	FMemoryWriter archive_(buffer);
	FArchive* archive = &archive_;

	if (archive->GetError())
	{
		UE_LOG(AnalyticsFTPLog, Error, TEXT("Capture could not be serialized"));
		return FAnalyticsCaptureInfo();
	}

	LocalPacketSerializer* serializer = new LocalPacketSerializer(archive);
	for (UAnalyticsPacket* packet : capture->packets)
	{
		serializer->AddPacket(packet);
	}

	delete serializer;
	archive->Flush();
	archive->Close();


	uint8* data = buffer.GetData();
	DWORD data_size = buffer.Num() * sizeof(uint8);

	// Create directory tree if needed
	if (!FtpSetCurrentDirectoryA(ftp_handle, TCHAR_TO_UTF8(*GetPath())))
	{
		TArray<FString> tree;
		FString path = GetPath();
		tree.Add(path);

		int32 position;
		while(path.FindLastChar(TCHAR('/'), position))
		{
			if (position > 0) {
				path = path.Mid(0, position);
				tree.Add(path);
			}
			else break;
		}

		Algo::Reverse(tree);

		for(FString dir : tree)
		{
			if (!FtpSetCurrentDirectoryA(ftp_handle, TCHAR_TO_UTF8(*dir)))
			{
				UE_LOG(AnalyticsFTPLog, Warning, TEXT("Creating directory: %s"), *dir);
				FtpCreateDirectoryA(ftp_handle, TCHAR_TO_UTF8(*dir));

				if (!FtpSetCurrentDirectoryA(ftp_handle, TCHAR_TO_UTF8(*dir)))
				{
					UE_LOG(AnalyticsFTPLog, Error, TEXT("Could not create directory on FTP server: %s"), *dir);
					return FAnalyticsCaptureInfo();
				}
			}
		}

		if (!FtpSetCurrentDirectoryA(ftp_handle, TCHAR_TO_UTF8(*GetPath())))
		{
			UE_LOG(AnalyticsFTPLog, Error, TEXT("Could not find directory on FTP server: %s"), *GetPath());
			return FAnalyticsCaptureInfo();
		}
	}

	FString filename = capture->Name + ".cap";
	HINTERNET file = FtpOpenFileA(ftp_handle, TCHAR_TO_UTF8(*filename), GENERIC_WRITE, FTP_TRANSFER_TYPE_BINARY, NULL);
	if (file == NULL)
	{
		UE_LOG(AnalyticsFTPLog, Error, TEXT("Could not upload data"));
		return FAnalyticsCaptureInfo();
	}
	DWORD written;
	if (!InternetWriteFile(file, data, data_size, &written)) { InternetCloseHandle(file); return FAnalyticsCaptureInfo(); }
	InternetCloseHandle(file);


	// Meta data

	if (capture->Meta.Num() != 0)
	{
		TArray<uint8> meta_buffer;
		FMemoryWriter meta_archive(meta_buffer);
		LocalPacketSerializer::StoreMetaData(&meta_archive, capture->Meta);

		data = meta_buffer.GetData();
		data_size = meta_buffer.Num() * sizeof(uint8);

		filename = capture->Name + ".meta";
		file = FtpOpenFileA(ftp_handle, TCHAR_TO_UTF8(*filename), GENERIC_WRITE, FTP_TRANSFER_TYPE_BINARY, NULL);
		if (file == NULL)
		{
			UE_LOG(AnalyticsFTPLog, Error, TEXT("Could not upload meta data"));
			return FAnalyticsCaptureInfo();
		}
		if (!InternetWriteFile(file, data, data_size, &written)) { InternetCloseHandle(file); return FAnalyticsCaptureInfo(); }
		InternetCloseHandle(file);

	}

	FAnalyticsCaptureInfo info;
	info.Name = capture->Name;
	info.Source = Connection;
	info.Meta = capture->Meta;

	capture->ConditionalBeginDestroy();

	return info;
}

UAnalyticsCapture* UAnalyticsFTPCaptureManager::DeserializeCapture(FAnalyticsCaptureInfo info)
{
	if(!FtpSetCurrentDirectoryA(ftp_handle, TCHAR_TO_UTF8(*GetPath())))
	{
		UE_LOG(AnalyticsFTPLog, Error, TEXT("Could not find directory on FTP server: %s"), *GetPath());
		return nullptr;
	}

	TArray<uint8> data;
	FString filename = info.Name + ".cap";
	HINTERNET file = FtpOpenFileA(ftp_handle, TCHAR_TO_UTF8(*filename), GENERIC_READ, FTP_TRANSFER_TYPE_BINARY, NULL);
	if (file == NULL)
	{
		UE_LOG(AnalyticsFTPLog, Error, TEXT("Could not download data"));
		return nullptr;
	}

	uint8 buffer[512];
	DWORD read = 1;
	while(InternetReadFile(file, &buffer, 512, &read) && read > 0)
	{
		for (uint32 i = 0; i < read; i++)
		{
			data.Add(buffer[i]);
		}
	}

	InternetCloseHandle(file);

	FMemoryReader archive = FMemoryReader(data, true);

	UAnalyticsCapture* capture = NewObject<UAnalyticsCapture>(this);
	capture->Name = info.Name;
	LocalPacketDeserializer deserializer(&archive, capture);
	deserializer.Process();

	capture->Meta = info.Meta;

	return capture;
}

TArray<FAnalyticsCaptureInfo> UAnalyticsFTPCaptureManager::FindCaptures()
{
	if (!FtpSetCurrentDirectoryA(ftp_handle, TCHAR_TO_UTF8(*GetPath())))
	{
		UE_LOG(AnalyticsFTPLog, Error, TEXT("Could not find directory on FTP server: %s"), *GetPath());
		return TArray<FAnalyticsCaptureInfo>();
	}

	WIN32_FIND_DATAA find;
	bool found = true;
	HINTERNET find_handle = FtpFindFirstFileA(ftp_handle, "*.cap", &find, INTERNET_FLAG_NEED_FILE, NULL);
	if (find_handle == NULL)
	{
		UE_LOG(AnalyticsFTPLog, Error, TEXT("Could not query captures"));
		return TArray<FAnalyticsCaptureInfo>();
	}

	TArray<FAnalyticsCaptureInfo> captures;

	while(found)
	{
		FAnalyticsCaptureInfo info;
		info.Name = FPaths::GetBaseFilename(UTF8_TO_TCHAR(find.cFileName));
		info.Source = Connection;
		
		FString filename = info.Name + ".meta";

		HINTERNET file = FtpOpenFileA(ftp_handle, TCHAR_TO_UTF8(*filename), GENERIC_READ, FTP_TRANSFER_TYPE_BINARY, NULL);
		if (file != NULL)
		{
			TArray<uint8> data;

			uint8 buffer[512];
			DWORD read = 1;
			while (InternetReadFile(file, &buffer, 512, &read) && read > 0)
			{
				for (uint32 i = 0; i < read; i++)
				{
					data.Add(buffer[i]);
				}
			}

			InternetCloseHandle(file);

			FMemoryReader archive = FMemoryReader(data, true);
			info.Meta = LocalPacketDeserializer::LoadMetaData(&archive);
			archive.Close();
		}

		captures.Add(info);

		found = InternetFindNextFileA(find_handle, &find) != 0;
	}

	InternetCloseHandle(find_handle);

	return captures;
}

void UAnalyticsFTPCaptureManager::DeleteStoredCapture(FAnalyticsCaptureInfo info)
{
	if (!FtpSetCurrentDirectoryA(ftp_handle, TCHAR_TO_UTF8(*GetPath())))
	{
		UE_LOG(AnalyticsFTPLog, Error, TEXT("Could not find directory on FTP server: %s"), *GetPath());
		return;
	}

	FString filename = info.Name + ".cap";
	FtpDeleteFileA(ftp_handle, TCHAR_TO_UTF8(*filename));

	filename = info.Name + ".meta";
	FtpDeleteFileA(ftp_handle, TCHAR_TO_UTF8(*filename));
}

UAnalyticsFTPCaptureManager* UAnalyticsFTPCaptureManager::ConnectToFTP(FString Adress, FString Username, FString Password, FString Directory, FTPConnectionResult& ConnectionResult)
{
	UAnalyticsFTPCaptureManager* manager = NewObject<UAnalyticsFTPCaptureManager>();
	manager->AddToRoot();
	manager->directory = Directory;
	manager->connection = InternetOpenA(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (manager->connection == NULL)
	{
		UE_LOG(AnalyticsFTPLog, Error, TEXT("Could not open internet interface"));
		ConnectionResult = FTPConnectionResult::Failed;
		manager->RemoveFromRoot(); 
		manager->ConditionalBeginDestroy();
		return nullptr;
	}

	manager->ftp_handle = InternetConnectA(manager->connection, TCHAR_TO_UTF8(*Adress), INTERNET_DEFAULT_FTP_PORT, TCHAR_TO_UTF8(*Username), TCHAR_TO_UTF8(*Password), INTERNET_SERVICE_FTP, 0, 0);
	if (manager->ftp_handle == NULL)
	{
		UE_LOG(AnalyticsFTPLog, Error, TEXT("Could not connect to FTP server"));
		InternetCloseHandle(manager->connection);
		ConnectionResult = FTPConnectionResult::Failed;
		manager->RemoveFromRoot();
		manager->ConditionalBeginDestroy();
		return nullptr;
	}

	ConnectionResult = FTPConnectionResult::Success;
	return manager;
}

void UAnalyticsFTPCaptureManager::ConnectToFTPLatent(UObject* WorldContextObject, FLatentActionInfo LatentInfo, const FString Adress, const FString Username, const FString Password, FString Directory, UAnalyticsFTPCaptureManager*& Result, FTPConnectionResult& ConnectionResult)
{
	if (UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FFTPConnectAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == NULL)
		{
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FFTPConnectAction(ConnectionResult, Result, Adress, Username, Password, Directory, LatentInfo));
		}
	}
}

FFTPConnectAction::FFTPConnectAction(FTPConnectionResult& Result, UAnalyticsFTPCaptureManager*& ReturnObject, FString Adress, FString Username, FString Password, FString Directory, const FLatentActionInfo& LatentInfo) : ExecutionFunction(LatentInfo.ExecutionFunction), OutputLink(LatentInfo.Linkage), CallbackTarget(LatentInfo.CallbackTarget)
{
	finish_flag = false;

	FFunctionGraphTask::CreateAndDispatchWhenReady([&Result, &ReturnObject, Adress, Username, Password, Directory, this]()
	{
		ReturnObject = UAnalyticsFTPCaptureManager::ConnectToFTP(Adress, Username, Password, Directory, Result);
		this->finish_flag = true;
	}, TStatId(), nullptr, ENamedThreads::AnyNormalThreadNormalTask);
}

void FFTPConnectAction::UpdateOperation(FLatentResponse& Response)
{
	Response.FinishAndTriggerIf(finish_flag, ExecutionFunction, OutputLink, CallbackTarget);
}

UAnalyticsCaptureManager* UAnalyticsFTPCaptureManagerInfo::GetManager()
{
	if(manager == nullptr)
	{
		FTPConnectionResult result;
		manager = UAnalyticsFTPCaptureManager::ConnectToFTP(Adress, Username, Password, Directory, result);
		if (manager != nullptr)
		{
			manager->Connection = TWeakObjectPtr<UAnalyticsCaptureManagerConnection>(this);
		}
	}

	return manager;
}

void UAnalyticsFTPCaptureManagerInfo::ReleaseManager()
{
	manager->Disconnect();
	manager = nullptr;
}

void UAnalyticsFTPCaptureManager::Disconnect()
{
	InternetCloseHandle(ftp_handle);
	InternetCloseHandle(connection);
	RemoveFromRoot();
	ConditionalBeginDestroy();
}

FString UAnalyticsFTPCaptureManager::GetPath()
{
	if (directory.IsEmpty()) return "/";
	return "/" + directory.Replace(TEXT("\\"), TEXT("/"));
}


