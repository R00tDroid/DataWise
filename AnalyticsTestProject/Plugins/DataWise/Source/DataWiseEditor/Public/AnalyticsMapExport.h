#pragma once
#include "AnalyticscompilationStage.h"
#include "AnalyticsMapExport.generated.h"

class UAnalyticsMap;

UCLASS()
class UAnalyticsMapChannel : public UObject {
public:
	GENERATED_BODY()

	FString ExportName;
	FString ChannelName;

	virtual FString Export(FString prefix) { return FString(); }

	UAnalyticsMap* ParentMap;
};

UCLASS()
class UAnalyticsHeatmap : public UAnalyticsMapChannel {
public:
GENERATED_BODY()

	UFUNCTION(BlueprintCallable)
	void WriteDensity(FVector2D Location, float Density);

	UFUNCTION(BlueprintCallable)
	void AddDensity(FVector2D Location, float Density);

	void Setup(FVector2D DensityResolution);

	FString Export(FString prefix) override;

private:
	TMap<FVector2D, float> DensityData;

	FVector2D DensityResolution;
};

UCLASS()
class UAnalyticsMap : public UAnalyticsExportableData {
public:
	GENERATED_BODY()

	void Setup(int Width, int Height, UTexture2D* Background, FVector2D WorldSize, FVector2D WorldCenter);

	void ExportData(FString prefix, uint32 unique_id, FString& output_data, FString& output_graph) override;

	UFUNCTION(BlueprintCallable)
	UAnalyticsHeatmap* AddDensityChannel(FString ChannelName, FVector2D DensityResolution = FVector2D(100, 100));

	FIntPoint GetSize() { return Size; }
	FVector2D GetWorldSize() { return WorldSize; }
	FVector2D GetWorldCenter() { return WorldCenter; }

private:
	TArray<UAnalyticsMapChannel*> channels;

	FIntPoint Size;
	UTexture2D* Background;
	FVector2D WorldSize;
	FVector2D WorldCenter;
};