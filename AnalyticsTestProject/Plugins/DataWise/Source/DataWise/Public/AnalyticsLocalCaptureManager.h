#pragma once
#include "DataWise.h"
#include "AnalyticsCaptureManager.h"
#include "AnalyticsPacket.h"
#include "AnalyticsLocalCaptureManager.generated.h"

typedef uint32 PacketTypeIndex;
typedef uint32 PacketPropertyCount;

#define local_capture_path "\\.Analytics\\Captures\\"
#define local_cache_path "\\.Analytics\\Cache\\"

UCLASS()
class DATAWISE_API UAnalyticsLocalCaptureManager : public UAnalyticsCaptureManager
{
GENERATED_BODY()
public:

	UFUNCTION(BLUEPRINTCALLABLE)
	static UAnalyticsLocalCaptureManager* GetLocalCaptureManager();
	
	FAnalyticsCaptureInfo SerializeCapture(UAnalyticsCapture* capture) override;
	FAnalyticsCaptureInfo SerializeCaptureToCache(UAnalyticsCapture* capture);

	

	UAnalyticsCapture* DeserializeCapture(FAnalyticsCaptureInfo info) override;

	TArray<FAnalyticsCaptureInfo> FindCaptures() override;
	TArray<FAnalyticsCaptureInfo> FindCachedCaptures();

	void DeleteStoredCapture(FAnalyticsCaptureInfo info) override;

	private:
	FAnalyticsCaptureInfo SerializeCaptureToDirectory(FString path, UAnalyticsCapture* capture);
};

UCLASS()
class DATAWISE_API UAnalyticsLocalCaptureManagerInfo : public UAnalyticsCaptureManagerConnection
{
	GENERATED_BODY()
public:
	UAnalyticsCaptureManager* GetManager() override { return manager; }

	UAnalyticsLocalCaptureManager* manager;
};



class DATAWISE_API LocalPacketSerializer
{
public:

	LocalPacketSerializer(FArchive*);
	void AddPacket(UAnalyticsPacket*);

	static void StoreMetaData(FArchive* Archive, TMap<FString, FString> Meta);

private:
	FArchive* archive;

	TMap<UClass*, PacketTypeIndex> packet_types;
	PacketTypeIndex next_packet_id = 1; // 0 is reserved for packet class registration

	PacketTypeIndex RegisterPacketType(TSubclassOf<UAnalyticsPacket> type);
};

class DATAWISE_API LocalPacketDeserializer
{
public:

	static TMap<FString, FString> LoadMetaData(FArchive* Archive);

	LocalPacketDeserializer(FArchive*, UAnalyticsCapture*);
	void Process();

private:
	FArchive* archive;
	UAnalyticsCapture* output;

	TMap<PacketTypeIndex, UClass*> packet_types;

	void RegisterPacketType();
};