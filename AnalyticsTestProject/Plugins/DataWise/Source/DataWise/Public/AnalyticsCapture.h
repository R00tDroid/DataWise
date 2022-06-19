#pragma once
#include "AnalyticsPacket.h"
#include "AnalyticsCapture.generated.h"

class UAnalyticsCaptureManagerConnection;

USTRUCT(BlueprintType)
struct DATAWISE_API FAnalyticsCaptureInfo
{
	GENERATED_BODY()

	FString Name = "";
	TWeakObjectPtr<UAnalyticsCaptureManagerConnection> Source;
	TMap<FString, FString> Meta;

	bool operator==(FAnalyticsCaptureInfo const& other) const
	{
		return Name.Equals(other.Name);
	}
};


UCLASS(BlueprintType)
class DATAWISE_API UAnalyticsCapture : public UObject
{
GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FString Name;

	UPROPERTY(BlueprintReadOnly)
	TMap<FString, FString> Meta;

	UPROPERTY(BlueprintReadOnly)
	TArray<UAnalyticsPacket*> packets;
};
