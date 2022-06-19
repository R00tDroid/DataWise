#include "AnalyticsLocalCaptureManager.h"
#include "DataWise.h"
#include "ClassFinder.h"
#include "Serialization/MemoryReader.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "AnalyticsPacket.h"


UAnalyticsLocalCaptureManager* local_capture_manager_ = nullptr;

UAnalyticsLocalCaptureManager* UAnalyticsLocalCaptureManager::GetLocalCaptureManager()
{
	if (local_capture_manager_ == nullptr)
	{
		local_capture_manager_ = NewObject<UAnalyticsLocalCaptureManager>();
		local_capture_manager_->AddToRoot();
	}

	return local_capture_manager_;
}

FAnalyticsCaptureInfo UAnalyticsLocalCaptureManager::SerializeCapture(UAnalyticsCapture* capture)
{
	return SerializeCaptureToDirectory(FPaths::ProjectDir() + local_capture_path, capture);
}

FAnalyticsCaptureInfo UAnalyticsLocalCaptureManager::SerializeCaptureToCache(UAnalyticsCapture* capture)
{
	return SerializeCaptureToDirectory(FPaths::ProjectDir() + local_cache_path, capture);
}

UAnalyticsCapture* UAnalyticsLocalCaptureManager::DeserializeCapture(FAnalyticsCaptureInfo info)
{
	FString directory = FPaths::ProjectDir() + local_capture_path;
	FString path = directory + info.Name + ".cap";

	if(!FPaths::FileExists(path))
	{
		directory = FPaths::ProjectDir() + local_cache_path;
		path = directory + info.Name + ".cap";
	}

	if (!FPaths::FileExists(path))
	{
		UE_LOG(AnalyticsLog, Error, TEXT("Capture not found in storage or cache: %s"), *info.Name);
		return nullptr;
	}

	// Capture

	TArray<uint8> file_data;
	FFileHelper::LoadFileToArray(file_data, *path);
	FMemoryReader archive = FMemoryReader(file_data, true);

	UAnalyticsCapture* capture = NewObject<UAnalyticsCapture>(this);
	capture->Name = info.Name;
	LocalPacketDeserializer deserializer(&archive, capture);
	deserializer.Process();

	capture->Meta = info.Meta;

	return capture;
}

TArray<FAnalyticsCaptureInfo> UAnalyticsLocalCaptureManager::FindCaptures()
{
	FString directory = FPaths::ProjectDir() + local_capture_path;

	TArray<FString> files;
	TArray<FAnalyticsCaptureInfo> result;

	IFileManager& FileManager = IFileManager::Get();
	FileManager.FindFiles(files, *directory, TEXT(".cap"));

	for (int i = 0; i < files.Num(); i++)
	{
		int64 size = FPlatformFileManager::Get().GetPlatformFile().FileSize(*(directory + files[i]));
		if(size <= 0) continue;

		FAnalyticsCaptureInfo info;
		info.Name = FPaths::GetBaseFilename(files[i]);
		info.Source = Connection;

		FString meta_path = directory + info.Name + ".meta";
		TArray<uint8> file_data;
		if (FFileHelper::LoadFileToArray(file_data, *meta_path, FILEREAD_Silent)) {
			FMemoryReader archive = FMemoryReader(file_data, true);

			info.Meta = LocalPacketDeserializer::LoadMetaData(&archive);

			archive.Close();
		}

		result.Add(info);
	}

	return result;
}

TArray<FAnalyticsCaptureInfo> UAnalyticsLocalCaptureManager::FindCachedCaptures()
{
	FString directory = FPaths::ProjectDir() + local_cache_path;

	TArray<FString> files;
	TArray<FAnalyticsCaptureInfo> result;

	IFileManager& FileManager = IFileManager::Get();
	FileManager.FindFiles(files, *directory, TEXT(".cap"));

	for (int i = 0; i < files.Num(); i++)
	{
		FAnalyticsCaptureInfo info;
		info.Name = FPaths::GetBaseFilename(files[i]);

		FString meta_path = directory + info.Name + ".meta";
		TArray<uint8> file_data;
		if (FFileHelper::LoadFileToArray(file_data, *meta_path, FILEREAD_Silent)) {
			FMemoryReader archive = FMemoryReader(file_data, true);

			info.Meta = LocalPacketDeserializer::LoadMetaData(&archive);

			archive.Close();
		}

		result.Add(info);
	}

	return result;
}

void UAnalyticsLocalCaptureManager::DeleteStoredCapture(FAnalyticsCaptureInfo info)
{
	FString directory = FPaths::ProjectDir() + local_capture_path;
	FString path = directory + info.Name + ".cap";
	FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*path);

	path = directory + info.Name + ".meta";
	FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*path);
}

FAnalyticsCaptureInfo UAnalyticsLocalCaptureManager::SerializeCaptureToDirectory(FString directory, UAnalyticsCapture* capture)
{
	// Capture

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*directory);
	FString path = directory + capture->Name + ".cap";
	

	FArchive* archive = IFileManager::Get().CreateFileWriter(*path);

	if (archive->GetError())
	{
		UE_LOG(AnalyticsLog, Error, TEXT("Capture could not be serialized"));
		return FAnalyticsCaptureInfo();
	}

	LocalPacketSerializer* serializer = new LocalPacketSerializer(archive);
	for(UAnalyticsPacket* packet : capture->packets)
	{
		serializer->AddPacket(packet);
	}

	delete serializer;
	archive->Flush();
	archive->Close();

	// Meta data

	if (capture->Meta.Num() != 0) 
	{
		path = directory + capture->Name + ".meta";

		PlatformFile.CreateDirectoryTree(*directory);

		archive = IFileManager::Get().CreateFileWriter(*path);

		if (archive == nullptr)
		{
			UE_LOG(AnalyticsLog, Error, TEXT("Could not write to %s"), *path);
			return FAnalyticsCaptureInfo();
		}

		if (archive->GetError())
		{
			UE_LOG(AnalyticsLog, Error, TEXT("Could not open meta file: %s"), *path);
			return FAnalyticsCaptureInfo();
		}

		LocalPacketSerializer::StoreMetaData(archive, capture->Meta);

		archive->Flush();
		archive->Close();
	}

	FAnalyticsCaptureInfo info;
	info.Name = capture->Name;
	info.Meta = capture->Meta;

	capture->ConditionalBeginDestroy();

	return info;
}





LocalPacketSerializer::LocalPacketSerializer(FArchive* archive_) : archive(archive_), next_packet_id(1)
{

}

void LocalPacketSerializer::AddPacket(UAnalyticsPacket* packet)
{
	if (archive == nullptr) 
	{
		UE_LOG(AnalyticsLog, Error, TEXT("No archive to write to!"));
		return;
	}

	if (packet == nullptr)
	{
		UE_LOG(AnalyticsLog, Error, TEXT("Reported packet is not valid!"));
		return;
	}

	UClass* reported_class = packet->GetClass();

	PacketTypeIndex packet_id = 0;
	PacketTypeIndex* packet_id_ptr = packet_types.Find(reported_class);
	if (packet_id_ptr == nullptr)
	{
		packet_id = RegisterPacketType(TSubclassOf<UAnalyticsPacket>(reported_class));
	}
	else {
		packet_id = *packet_id_ptr;
	}

	*archive << packet_id;

	for (TFieldIterator<UProperty> property_iterator(reported_class); property_iterator; ++property_iterator)
	{
		UProperty* prop = *property_iterator;
		FString type = prop->GetCPPType();
		FString name = prop->GetNameCPP();
		FString value;

		if (type.Equals("FString")) {
			FString* data = prop->ContainerPtrToValuePtr<FString>(packet);
			*archive << *data;
		}
		else if (type.Equals("FVector")) {
			FVector* data = prop->ContainerPtrToValuePtr<FVector>(packet);
			*archive << *data;
		}
		else if (type.Equals("int32")) {
			int32* data = prop->ContainerPtrToValuePtr<int32>(packet);
			*archive << *data;
		}
		else if (type.Equals("uint32")) {
			uint32* data = prop->ContainerPtrToValuePtr<uint32>(packet);
			*archive << *data;
		}
		else if (type.Equals("uint8")) {
			uint8* data = prop->ContainerPtrToValuePtr<uint8>(packet);
			*archive << *data;
		}
		else if (type.Equals("bool")) {
			bool* data = prop->ContainerPtrToValuePtr<bool>(packet);
			*archive << *data;
		}
		else if (type.Equals("float")) {
			float* data = prop->ContainerPtrToValuePtr<float>(packet);
			*archive << *data;
		}
		else if (type.Equals("FVector2D")) {
			FVector2D* data = prop->ContainerPtrToValuePtr<FVector2D>(packet);
			*archive << *data;
		}
		else
		{
			UE_LOG(AnalyticsLog, Error, TEXT("Unsupported property type: %s (%s)"), *type, *name);
		}
	}
}

void LocalPacketSerializer::StoreMetaData(FArchive * Archive, TMap<FString, FString> Meta)
{
	TArray<FString> keys;
	Meta.GetKeys(keys);
	for (FString key : keys)
	{
		*Archive << key;
		*Archive << Meta[key];
	}
}

PacketTypeIndex LocalPacketSerializer::RegisterPacketType(TSubclassOf<UAnalyticsPacket> type)
{
	PacketTypeIndex register_class_type = 0;
	*archive << register_class_type;	// Class registration packet id
	*archive << next_packet_id;			// ID to register
	FString name = type.Get()->GetName();
	*archive << name;					// Classname to register

	PacketPropertyCount property_count = 0;

	for (TFieldIterator<UProperty> property_iterator(*type); property_iterator; ++property_iterator)
	{
		property_count++;
	}

	*archive << property_count;			// Amount of properties

	for (TFieldIterator<UProperty> property_iterator(*type); property_iterator; ++property_iterator)
	{
		UProperty* prop = *property_iterator;
		FString prop_type = prop->GetCPPType();
		FString prop_name = prop->GetNameCPP();
		*archive << prop_name;				// Property name
		*archive << prop_type;				// Property type
	}

	packet_types.Add(*type, next_packet_id);
	PacketTypeIndex registered_id = next_packet_id;
	next_packet_id++;
	return registered_id;
}





TMap<FString, FString> LocalPacketDeserializer::LoadMetaData(FArchive* Archive)
{
	TMap<FString, FString> meta;

	FString key, value;

	while (Archive->Tell() < Archive->TotalSize() - 1)
	{
		*Archive << key;
		*Archive << value;
		meta.Add(key, value);
	}

	return meta;
}

LocalPacketDeserializer::LocalPacketDeserializer(FArchive* archive_, UAnalyticsCapture* capture) : archive(archive_), output(capture)
{

}

void LocalPacketDeserializer::Process()
{
	PacketTypeIndex packet_type;

	while (archive->Tell() < archive->TotalSize() - 1)
	{
		*archive << packet_type;

		if (packet_type == 0) { RegisterPacketType(); continue; }

		UClass** type_ptr = packet_types.Find(packet_type);

		if(type_ptr==nullptr)
		{
			UE_LOG(AnalyticsLog, Error, TEXT("Packet type not found: %i"), packet_type);
			return;
		}

		UClass* class_type = *type_ptr;
		UAnalyticsPacket* packet = NewObject<UAnalyticsPacket>(output, class_type);
		output->packets.Add(packet);

		for (TFieldIterator<UProperty> property_iterator(class_type); property_iterator; ++property_iterator)
		{
			UProperty* prop = *property_iterator;
			FString type = prop->GetCPPType();
			FString name = prop->GetNameCPP();

			if (type.Equals("FString")) {
				FString* data = prop->ContainerPtrToValuePtr<FString>(packet);
				*archive << *data;
			}
			else if (type.Equals("FVector")) {
				FVector* data = prop->ContainerPtrToValuePtr<FVector>(packet);
				*archive << *data;
			}
			else if (type.Equals("int32")) {
				int32* data = prop->ContainerPtrToValuePtr<int32>(packet);
				*archive << *data;
			}
			else if (type.Equals("uint32")) {
				uint32* data = prop->ContainerPtrToValuePtr<uint32>(packet);
				*archive << *data;
			}
			else if (type.Equals("uint8")) {
				uint8* data = prop->ContainerPtrToValuePtr<uint8>(packet);
				*archive << *data;
			}
			else if (type.Equals("bool")) {
				bool* data = prop->ContainerPtrToValuePtr<bool>(packet);
				*archive << *data;
			}
			else if (type.Equals("float")) {
				float* data = prop->ContainerPtrToValuePtr<float>(packet);
				*archive << *data;
			}
			else if (type.Equals("FVector2D")) {
				FVector2D* data = prop->ContainerPtrToValuePtr<FVector2D>(packet);
				*archive << *data;
			}
			else
			{
				UE_LOG(AnalyticsLog, Error, TEXT("Unsupported property type: %s (%s)"), *type, *name); break;
			}
		}
	}
}

void LocalPacketDeserializer::RegisterPacketType()
{
	PacketTypeIndex register_id;
	*archive << register_id;
	
	if(packet_types.Find(register_id) != nullptr)
	{
		UE_LOG(AnalyticsLog, Error, TEXT("Packet type already exists! index: %i"), register_id);
		return;
	}

	FString class_name;
	*archive << class_name;

	TArray<UClass*> possible_classes = UClassFinder::FindSubclasses(UAnalyticsPacket::StaticClass());

	UClass* found_class = nullptr;

	for(UClass* possible_class : possible_classes)
	{
		if (possible_class->GetName().Equals(class_name)) { found_class = possible_class; break; }
	}

	if(found_class == nullptr)
	{
		UE_LOG(AnalyticsLog, Error, TEXT("No matching class found! name: %s"), *class_name);
		return;
	}

	PacketPropertyCount property_count;
	*archive << property_count;

	TArray<FString> property_names;
	TArray<FString> property_types;

	for (PacketPropertyCount i = 0; i < property_count; i++)
	{
		FString name;
		FString type;
		*archive << name;
		*archive << type;

		property_names.Add(name);
		property_types.Add(type);
	}

	uint32 property_id = 0;
	for (TFieldIterator<UProperty> property_iterator(found_class); property_iterator; ++property_iterator)
	{
		UProperty* prop = *property_iterator;
		FString type = prop->GetCPPType();
		FString name = prop->GetNameCPP();

		if (property_names.IsValidIndex(property_id)) 
		{
			if (!name.ToLower().Equals(property_names[property_id].ToLower()) || !type.ToLower().Equals(property_types[property_id].ToLower()))
			{
				UE_LOG(AnalyticsLog, Error, TEXT("Property does not match signature! Class: %s (%s), Capture: %s (%s)"), *name, *type, *property_names[property_id], *property_types[property_id]); 
				return;
			}
		} else
		{
			UE_LOG(AnalyticsLog, Error, TEXT("Class has more properties than registered"), *class_name); 
			return;
		}

		property_id++;
	}

	if (property_id != property_names.Num())
	{
		UE_LOG(AnalyticsLog, Error, TEXT("Class has less properties than registered!"), *class_name);
		return;
	}

	packet_types.Add(register_id, found_class);
}

