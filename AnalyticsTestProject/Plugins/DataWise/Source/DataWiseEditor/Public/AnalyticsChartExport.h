#pragma once
#include "AnalyticsCompilationStage.h"
#include "AnalyticsChartExport.generated.h"




UCLASS()
class UAnalyticsTable : public UAnalyticsExportableData {
public:
	GENERATED_BODY()

	void Setup(TArray<EChartType> ChartTypes, TMap<FString, ETableDataType> Columns);

	void ExportData(FString prefix, uint32 unique_id, FString& output_data, FString& output_graph) override;

	UFUNCTION(BlueprintCallable)
	void AddRow(TArray<FString> ColumnData);

private:
	TArray<FString> column_names;
	TArray<ETableDataType> column_types;
	TArray<EChartType> chart_types;

	TArray<TArray<FString>> rows;
};