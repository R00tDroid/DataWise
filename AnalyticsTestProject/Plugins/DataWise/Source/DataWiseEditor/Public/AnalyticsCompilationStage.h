#pragma once
#include "AnalyticsPacket.h"
#include "Templates/Casts.h"
#include "AnalyticscompilationStage.generated.h"

#define local_data_output "\\.Analytics\\Output\\"

class UAnalyticsMap;
class UAnalyticsTable;
class UAnalyticsSheet;

UENUM(BlueprintType) enum class EChartType : uint8 {
	Table,
	Line,
	Bar,
	Column,
	Pie,
	Scatter,
	Timeline
};

UENUM(BlueprintType) enum class ETableDataType : uint8 {
	String,
	Number,
	Boolean,
	Seconds,
	Milliseconds
};

UENUM(BlueprintType) enum class ESheetDataType : uint8 {
	String,
	Number,
	Boolean,
	Time
};


UCLASS()
class UAnalyticsExportableData : public UObject {
public:
	GENERATED_BODY()
	virtual void ExportData(FString prefix, uint32 unique_id, FString& output_data, FString& output_graph){};

	FString ExportName;
};

UCLASS(Blueprintable)
class DATAWISEEDITOR_API UAnalyticsCompilerContext : public UObject {
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	TArray<UAnalyticsPacket*> Packets;

	UPROPERTY(BlueprintReadOnly)
	FString Name;

	UFUNCTION(BlueprintCallable)
	TArray<UAnalyticsPacket*> GetAllPackets();

	UFUNCTION(BlueprintCallable)
	TArray<UAnalyticsPacket*> GetPacketsInTimeFrame(int32 start, int32 end);

	UFUNCTION(BlueprintCallable)
	TArray<UAnalyticsPacket*> GetPacketsOfType(TSubclassOf<UAnalyticsPacket> type);
};

UCLASS(Blueprintable) 
class UAnalyticsCompilationStage : public UObject {
GENERATED_BODY()
public:

	UFUNCTION(BlueprintImplementableEvent)
	void PreProcessCapture();

	UFUNCTION(BlueprintImplementableEvent)
	void ProcessCapture(UAnalyticsCompilerContext* Context);

	UFUNCTION(BlueprintImplementableEvent)
	void PostProcessCapture();


	UFUNCTION(BlueprintImplementableEvent)
	void PreProcessCaptureGroup();

	UFUNCTION(BlueprintImplementableEvent)
	void ProcessCaptureGroup(UAnalyticsCompilerContext* Context);

	UFUNCTION(BlueprintImplementableEvent)
	void PostProcessCaptureGroup();

	void ExportData(FString Name);

	void BeginDestroy() override;

protected:
	UFUNCTION(BlueprintCallable)
	UAnalyticsMap* CreateMap(FString Name, int Width, int Height, UTexture2D* Background, FVector2D WorldSize, FVector2D WorldCenter = FVector2D(0, 0));

	UFUNCTION(BlueprintCallable)
	UAnalyticsTable* CreateTable(FString Name, TArray<EChartType> ChartTypes, TMap<FString, ETableDataType> ColumnsNamesAndTypes);

	UFUNCTION(BlueprintCallable)
	UAnalyticsSheet* CreateSheet(FString Name, TMap<FString, ESheetDataType> ColumnsNamesAndTypes, FString Seperator = ";");

private:
	TArray<UAnalyticsExportableData*> exporters;
};




class BitmapRenderer
{
public:
	BitmapRenderer(int w, int h);

	void Clear(FColor);
	void DrawPixel(FIntPoint, FColor);
	void DrawTriangle(FIntPoint a, FIntPoint b, FIntPoint c, FColor);
	void DrawTriangle(FIntPoint a, FIntPoint b, FIntPoint c, FColor ca, FColor cb, FColor cc);
	void DrawLine(FIntPoint a, FIntPoint b, FColor c);
	void DrawRectangle(FIntPoint pos, FIntPoint size, FColor);
	void DrawRectangle(FIntPoint a, FIntPoint b, FIntPoint c, FIntPoint d, FColor);
	void DrawRectangle(FIntPoint a, FIntPoint b, FIntPoint c, FIntPoint d, FColor ca, FColor cb, FColor cc, FColor cd);
	void DrawImage(FIntPoint pos, FIntPoint size, UTexture2D* texture);
	void Export(FString);

private:
	uint32 LocalToIndex(FIntPoint);
	FIntPoint IndexToLocal(uint32);

	TArray<FColor> BitmapFromTexture(UTexture2D* texture);

	TArray<FColor> data;
	FIntPoint size;
};