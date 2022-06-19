#pragma once
#include "UObject/Class.h"
#include "AnalyticsPacket.h"
#include "AnalyticsSession.generated.h"

class LocalPacketSerializer;

UCLASS()
class DATAWISE_API UAnalyticsSession : public UObject
{
GENERATED_BODY()
public:

	UFUNCTION(BLUEPRINTCALLABLE, BLUEPRINTPURE, Category = "Analytics Session")
	static void GetSession(int32 index, UAnalyticsSession*& session, bool& valid);

	UFUNCTION(BLUEPRINTCALLABLE, BLUEPRINTPURE, Category = "Analytics Session")
	static TArray<UAnalyticsSession*> GetActiveSessions();

	UFUNCTION(BLUEPRINTCALLABLE, Category = "Analytics Session", meta = (WorldContext = WorldContextObject))
	static void BeginSession(UObject* WorldContextObject, FString name, UAnalyticsSession*& session, int32& index);

	UFUNCTION(BLUEPRINTCALLABLE, Category = "Analytics Session")
	void EndSession();

	UFUNCTION(BLUEPRINTCALLABLE, Category = "Analytics Session")
	void SetMetaData(TMap<FString, FString> Meta);

	UFUNCTION(BLUEPRINTCALLABLE, meta = (BlueprintInternalUseOnly = "true"))
	void ReportEvent(UObject* packet);

	virtual void BeginDestroy() override;

	UFUNCTION(BLUEPRINTCALLABLE, Category = "Analytics Session")
	FString GetName() { return name; }

	UFUNCTION(BLUEPRINTCALLABLE, Category = "Analytics Session")
	int32 GetCaptureSize();

	UFUNCTION(BLUEPRINTCALLABLE, Category = "Analytics Session")
	FString GetFormattedCaptureSize();

private:
	double start_time;

	FString name;

	bool active = false;

	FArchive* archive;

	LocalPacketSerializer* serializer;

	TMap<FString, FString> meta_data;

	static TArray<UAnalyticsSession*> active_sessions;

	void StoreMetaData();
};