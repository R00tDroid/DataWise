#pragma once

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/MinWindows.h"
#include "Windows/WindowsSystemIncludes.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wininet.h>
#include "Windows/HideWindowsPlatformTypes.h"

#include "Engine/LatentActionManager.h"
#include "LatentActions.h"
#include "AnalyticsCaptureManager.h"
#include "AnalyticsFTPCaptureManager.generated.h"

UENUM(BlueprintType)
enum class FTPConnectionResult : uint8
{
	Success,
	Failed
};

UCLASS(BlueprintType)
class DATAWISEFTP_API UAnalyticsFTPCaptureManager : public UAnalyticsCaptureManager
{
GENERATED_BODY()

public:

	FAnalyticsCaptureInfo SerializeCapture(UAnalyticsCapture* capture) override;
	UAnalyticsCapture* DeserializeCapture(FAnalyticsCaptureInfo info) override;

	TArray<FAnalyticsCaptureInfo> FindCaptures() override;

	void DeleteStoredCapture(FAnalyticsCaptureInfo info) override;

	static UAnalyticsFTPCaptureManager* ConnectToFTP(FString Adress, FString Username, FString Password, FString Directory, FTPConnectionResult& ConnectionResult);

	UFUNCTION(BlueprintCallable, DisplayName = "Connect To FTP", Meta = (ExpandEnumAsExecs = "ConnectionResult", Latent, LatentInfo = "LatentInfo", HidePin = "WorldContextObject", WorldContext = "WorldContextObject"))
	static void ConnectToFTPLatent(UObject* WorldContextObject, struct FLatentActionInfo LatentInfo, const FString Adress, const FString Username, const FString Password, FString Directory, UPARAM(DisplayName = "FTP Connection") UAnalyticsFTPCaptureManager*& Result, FTPConnectionResult& ConnectionResult);

	UFUNCTION(BlueprintCallable)
	void Disconnect();

private:
	FString GetPath();

	HINTERNET connection;
	HINTERNET ftp_handle;
	FString directory;
};

class FFTPConnectAction : public FPendingLatentAction
{
public:
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;

	FThreadSafeBool finish_flag;

	FFTPConnectAction(FTPConnectionResult& Result, UAnalyticsFTPCaptureManager*& ReturnObject, FString Adress, FString Username, FString Password, FString Directory, const FLatentActionInfo& LatentInfo);

	virtual void UpdateOperation(FLatentResponse& Response) override;
};


UCLASS()
class DATAWISEFTP_API UAnalyticsFTPCaptureManagerInfo : public UAnalyticsCaptureManagerConnection 
{
	GENERATED_BODY()
public:
	FString GetTag() override { return "FTP"; }
	FString GetInfo() override { return Adress + ((!Username.IsEmpty()) ? ("#" + Username) : "") + ((!Directory.IsEmpty()) ? ("&" + Directory) : ""); }
	UAnalyticsCaptureManager* GetManager() override;
	void ReleaseManager() override;

	UPROPERTY()
	FString Adress = "";

	UPROPERTY()
	FString Username = "";

	UPROPERTY(meta=(DisplayName="pwd"))
	FString Password = "";

	UPROPERTY()
	FString Directory = "";

private:
	UAnalyticsFTPCaptureManager* manager = nullptr;
};

REGISTER_CAPTUREMANAGER_TAG(UAnalyticsFTPCaptureManagerInfo, "FTP")
