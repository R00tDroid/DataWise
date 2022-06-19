#pragma once
#include "AnalyticsCapture.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/LatentActionManager.h"
#include "LatentActions.h"
#include "HAL/ThreadSafeBool.h"
#include "AnalyticsCaptureManager.generated.h"

class UAnalyticsCaptureManagerConnection;

UCLASS()
class DATAWISE_API UAnalyticsCaptureManager : public UObject
{
GENERATED_BODY()
public:
	UFUNCTION(BLUEPRINTCALLABLE)
	virtual FAnalyticsCaptureInfo SerializeCapture(UAnalyticsCapture* capture) { return FAnalyticsCaptureInfo(); }

	UFUNCTION(BLUEPRINTCALLABLE)
	virtual UAnalyticsCapture* DeserializeCapture(FAnalyticsCaptureInfo info) { return nullptr; }

	UFUNCTION(BLUEPRINTCALLABLE)
	virtual TArray<FAnalyticsCaptureInfo> FindCaptures() { return TArray<FAnalyticsCaptureInfo>(); }

	virtual void DeleteStoredCapture(FAnalyticsCaptureInfo info) { };

	TWeakObjectPtr<UAnalyticsCaptureManagerConnection> Connection;
};

UCLASS()
class DATAWISE_API UAnalyticsCaptureManagerConnection : public UObject
{
	GENERATED_BODY()
public:
	virtual FString GetTag() { return FString("NoName"); };
	virtual FString GetInfo() { return FString(); };
	virtual UAnalyticsCaptureManager* GetManager() { return nullptr; };
	virtual void ReleaseManager() {};

	void LoadData(FArchive& archive);
	void StoreData(FArchive& archive);
};

UCLASS()
class DATAWISE_API UAnalyticsCaptureManagementTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static void TransferCapture(FAnalyticsCaptureInfo info, UAnalyticsCaptureManager* source, UAnalyticsCaptureManager* destination);

	UFUNCTION(BlueprintCallable, DisplayName = "Transfer Capture", Meta = (Latent, LatentInfo = "LatentInfo", HidePin = "WorldContextObject", WorldContext = "WorldContextObject"))
	static void TransferCapture_Latent(UObject* WorldContextObject, struct FLatentActionInfo LatentInfo, FAnalyticsCaptureInfo info, UAnalyticsCaptureManager* source, UAnalyticsCaptureManager* destination);


	static void TransferAllCaptures(UAnalyticsCaptureManager* source, UAnalyticsCaptureManager* destination);

	UFUNCTION(BlueprintCallable, DisplayName = "Transfer All Captures", Meta = (Latent, LatentInfo = "LatentInfo", HidePin = "WorldContextObject", WorldContext = "WorldContextObject"))
	static void TransferAllCaptures_Latent(UObject* WorldContextObject, struct FLatentActionInfo LatentInfo, UAnalyticsCaptureManager* source, UAnalyticsCaptureManager* destination);

	static void CacheCapture(FAnalyticsCaptureInfo info, UAnalyticsCaptureManager* source);

	static void LoadCaptureManagerConnections();
	static void SaveCaptureManagerConnections();
	static TArray<UAnalyticsCaptureManagerConnection*> GetCaptureManagerConnections();
	static void RegisterCaptureManagerConnection(UAnalyticsCaptureManagerConnection*);
	static void UnregisterCaptureManagerConnection(UAnalyticsCaptureManagerConnection*);
	static TMap<FString, UClass*> GetCaptureManagerConnectionTags();

	static void SetSelectedInfos(TArray<FAnalyticsCaptureInfo>);
	static TArray<FAnalyticsCaptureInfo> GetSelectedInfos();

	static void LoadSelectedCompilationStages();
	static void SaveSelectedCompilationStages();
	static void SetSelectedCompilationStages(TArray<UClass*>);
	static TArray<UClass*> GetSelectedCompilationStages();

	static void LoadSessionFilters();
	static void SaveSessionFilters();
	static void SetSessionFilters(FString);
	static FString GetSessionFilters();


	DECLARE_MULTICAST_DELEGATE(FOnSelectionChanged);
	DECLARE_MULTICAST_DELEGATE(FOnConnectionsChanged);
	static FOnSelectionChanged OnSelectionChanged;
	static FOnConnectionsChanged OnConnectionsChanged;
};


#define REGISTER_CAPTUREMANAGER_TAG(TYPE, NAME) namespace { CaptureManagerTagRegistration TYPE ## _implreg(TYPE ## ::StaticClass(), NAME); };

struct DATAWISE_API CaptureManagerTagRegistration
{
	CaptureManagerTagRegistration(UClass*, FString);
};




class FTransferCaptureAction : public FPendingLatentAction
{
public:
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;

	FThreadSafeBool finish_flag;

	FTransferCaptureAction(FAnalyticsCaptureInfo Info, UAnalyticsCaptureManager* Source, UAnalyticsCaptureManager* Destination, const FLatentActionInfo& LatentInfo);

	virtual void UpdateOperation(FLatentResponse& Response) override;
};

class FTransferAllCapturesAction : public FPendingLatentAction
{
public:
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;

	FThreadSafeBool finish_flag;

	FTransferAllCapturesAction(UAnalyticsCaptureManager* Source, UAnalyticsCaptureManager* Destination, const FLatentActionInfo& LatentInfo);

	virtual void UpdateOperation(FLatentResponse& Response) override;
};