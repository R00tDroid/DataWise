#pragma once
#include "UObject/Class.h"
#include "AnalyticsPacket.generated.h"


UCLASS(Blueprintable)
class DATAWISE_API UAnalyticsPacket : public UObject
{
GENERATED_BODY()
public:
	UPROPERTY()
	uint32 Time;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	int32 GetTime() { return Time; }
};