#include "AnalyticsSession.h"
#include "DataWise.h"
#include "AnalyticsLocalCaptureManager.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

TArray<UAnalyticsSession*> UAnalyticsSession::active_sessions;

void UAnalyticsSession::GetSession(int32 index, UAnalyticsSession*& session, bool& valid)
{
	valid = active_sessions.IsValidIndex(index);

	if (valid)
	{
		session = active_sessions[index];
	}
	else
	{
		session = nullptr;
	}
}

TArray<UAnalyticsSession*> UAnalyticsSession::GetActiveSessions()
{
	return active_sessions;
}

void UAnalyticsSession::BeginSession(UObject* WorldContextObject, FString name, UAnalyticsSession*& session, int32& index)
{
	UWorld * world = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	session = NewObject<UAnalyticsSession>(world);

	session->start_time = FPlatformTime::Seconds();
	index = active_sessions.Add(session);

	FDateTime now = FDateTime::Now();
	session->name = name + (name.IsEmpty() ? "" : "_") + FString::Printf(TEXT("%02i"), now.GetDay()) + FString::Printf(TEXT("%02i"), now.GetMonth()) + "_" + FString::Printf(TEXT("%02i"), now.GetHour()) + FString::Printf(TEXT("%02i"), now.GetMinute()) + FString::Printf(TEXT("%02i"), now.GetSecond());

	FString directory = FPaths::ProjectDir() + local_capture_path;
	FString filename = session->name + ".cap";
	FString path = directory + filename;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*directory);

	session->archive = IFileManager::Get().CreateFileWriter(*path);

	if (session->archive->GetError())
	{
		UE_LOG(AnalyticsLog, Error, TEXT("Session could not be started"));
		session->ConditionalBeginDestroy();
		session = nullptr;
		index = -1;
		return;
	}

	session->active = true;
	session->serializer = new LocalPacketSerializer(session->archive);

	UE_LOG(AnalyticsLog, Log, TEXT("Started analytics session"));
}

void UAnalyticsSession::EndSession()
{
	delete serializer;
	serializer = nullptr;

	archive->Flush();
	archive->Close();
	archive = nullptr;

	StoreMetaData();

	UE_LOG(AnalyticsLog, Log, TEXT("Ended analytics session"));
	active_sessions.Remove(this);
	active = false;
	ConditionalBeginDestroy();
}

void UAnalyticsSession::SetMetaData(TMap<FString, FString> Meta)
{
	TArray<FString> keys;
	Meta.GetKeys(keys);
	for(FString key : keys)
	{
		if (meta_data.Contains(key))
		{
			meta_data[key] = Meta[key];
		}
		else
		{
			meta_data.Add(key, Meta[key]);
		}
	}
}

void UAnalyticsSession::BeginDestroy()
{
	Super::BeginDestroy();
	if (active) EndSession();
}

int32 UAnalyticsSession::GetCaptureSize()
{
	if (archive == nullptr) return 0;
	return archive->Tell();
}

FString UAnalyticsSession::GetFormattedCaptureSize()
{
	int32 size = GetCaptureSize();
	FString out;
	if (size > 1024 * 1024 * 1024)
	{
		return FString::Printf(TEXT("%.2f"), size / 1024.0f) + "GB";
	}

	if (size > 1024 * 1024)
	{
		return FString::Printf(TEXT("%.2f"), size / 1024.0f) + "MB";
	}

	if (size > 1024)
	{
		return FString::Printf(TEXT("%.2f"), size / 1024.0f) + "KB";
	}

	return FString::FromInt(size) + "B";
}

void UAnalyticsSession::StoreMetaData()
{
	if (meta_data.Num() == 0) return;

	FString directory = FPaths::ProjectDir() + local_capture_path;
	FString filename = name + ".meta";
	FString path = directory + filename;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*directory);

	FArchive* meta_archive = IFileManager::Get().CreateFileWriter(*path);

	if (meta_archive == nullptr)
	{
		UE_LOG(AnalyticsLog, Error, TEXT("Could not write to %s"), *path);
		return;
	}

	if(meta_archive->GetError())
	{
		UE_LOG(AnalyticsLog, Error, TEXT("Could not open meta file: %s"), *path);
		return;
	}

	LocalPacketSerializer::StoreMetaData(meta_archive, meta_data);

	meta_archive->Flush();
	meta_archive->Close();
}

void UAnalyticsSession::ReportEvent(UObject* packet)
{
	if (archive == nullptr)
	{
		UE_LOG(AnalyticsLog, Error, TEXT("No archive to write to!"));
		packet->Rename(TEXT("Packet"), nullptr, REN_None);
		packet->ConditionalBeginDestroy();
		return;
	}

	if (packet == nullptr)
	{
		UE_LOG(AnalyticsLog, Error, TEXT("Reported packet is not valid!"));
		packet->Rename(TEXT("Packet"), nullptr, REN_None);
		packet->ConditionalBeginDestroy();
		return;
	}

	UClass* reported_class = packet->GetClass();
	if (!reported_class->IsChildOf(UAnalyticsPacket::StaticClass()))
	{
		UE_LOG(AnalyticsLog, Error, TEXT("Reported class is not a valid packet! Type: %s"), *reported_class->GetName());
		packet->Rename(TEXT("Packet"), nullptr, REN_None);
		packet->ConditionalBeginDestroy();
		return;
	}

	UAnalyticsPacket* reported_packet = Cast<UAnalyticsPacket>(packet);

	double time = FPlatformTime::Seconds() - start_time;
	reported_packet->Time = time;

	serializer->AddPacket(reported_packet);

	packet->Rename(TEXT("Packet"), nullptr, REN_None);
	packet->ConditionalBeginDestroy();
}
