#pragma once
#include "AnalyticsCompilationStage.h"
#include "AnalyticsSheetExport.generated.h"


UCLASS()
class UAnalyticsSheet : public UAnalyticsExportableData {
public:
	GENERATED_BODY()

	void Setup(FString Seperator, TMap<FString, ESheetDataType> Columns);

	void ExportData(FString, uint32, FString&, FString&) override;

	UFUNCTION(BlueprintCallable)
	void AddRow(TArray<FString> ColumnData);

private:
	FString seperator_type;
	TArray<FString> column_names;
	TArray<ESheetDataType> column_types;

	TArray<TArray<FString>> rows;
};