#include "AnalyticsChartExport.h"
#include "DataWiseEditor.h"


void UAnalyticsTable::Setup(TArray<EChartType> ChartTypes, TMap<FString, ETableDataType> Columns)
{
	Columns.GetKeys(column_names);
	for (FString column : column_names)
	{
		column_types.Add(Columns[column]);
	}

	chart_types = ChartTypes;
}

void UAnalyticsTable::AddRow(TArray<FString> ColumnData)
{
	if (ColumnData.Num() != column_types.Num()) { UE_LOG(AnalyticsLogEditor, Error, TEXT("Added column data does not match signature")); return; }
	rows.Add(ColumnData);
}

void UAnalyticsTable::ExportData(FString prefix, uint32 unique_id, FString& output_data, FString& output_graph)
{
	// Write columns

	FString data_name = "data" + FString::FromInt(unique_id);
	output_data += "var " + data_name + " = new google.visualization.DataTable();";
	for (int i = 0; i < column_names.Num(); i++) {
		FString type;
		switch (column_types[i])
		{
		case ETableDataType::String: { type = "string"; break; }
		case ETableDataType::Number: { type = "number"; break; }
		case ETableDataType::Boolean: { type = "boolean"; break; }
		case ETableDataType::Seconds: { type = "timeofday"; break; }
		case ETableDataType::Milliseconds: { type = "number"; break; }
		}
		output_data += data_name + ".addColumn('" + type + "', '" + column_names[i] + "');";
	}


	// Write data

	output_data += data_name + ".addRows([";
	for (TArray<FString> row : rows)
	{
		output_data += "[";

		for (int id = 0; id < row.Num(); id++)
		{
			switch (column_types[id])
			{
			case ETableDataType::String: { output_data += "'" + row[id] + "'"; break; }
			case ETableDataType::Number: { output_data += "{v: " + row[id] + ", f: '" + row[id] + "'}";  break; }
			case ETableDataType::Boolean: { output_data += row[id]; break; }
			case ETableDataType::Seconds:
			{
				int seconds = FCString::Atoi(*row[id]);
				int minutes = FMath::FloorToInt(seconds / 60.0f);
				int hours = FMath::FloorToInt(minutes / 60.0f);

				seconds -= minutes * 60;
				minutes -= hours * 60;

				output_data += "[" + FString::FromInt(hours) + "," + FString::FromInt(minutes) + "," + FString::FromInt(seconds) + ",0]"; break;
			}

			case ETableDataType::Milliseconds: { output_data += "{v: " + row[id] + ", f: '" + row[id] + "'}";  break; }
			}

			if (id < row.Num() - 1) { output_data += ","; }
		}

		output_data += "],";
	}
	output_data += "]);";

	// Write charts

	output_graph += "<div class='chart_parent'>";
	output_graph += "<h1>" + ExportName + "</h1>";

	uint32 chart_id = 0;
	for (EChartType chart : chart_types)
	{
		FString chart_name = "chart" + FString::FromInt(unique_id) + "_" + FString::FromInt(chart_id);
		FString chart_id_name = "chart_id" + FString::FromInt(unique_id) + "_" + FString::FromInt(chart_id);
		output_graph += "<div class='chart' id='" + chart_name + "' style='" + ((chart == EChartType::Table) ? "max-" : "") + "width: 800px; height: 500px;'></div>";

		switch (chart) {
		case EChartType::Table: { output_data += "var " + chart_id_name + "=new google.visualization.Table(document.getElementById('" + chart_name + "')); " + chart_id_name + ".draw(" + data_name + "); "; break; }
		case EChartType::Line: { output_data += "var " + chart_id_name + "=new google.visualization.LineChart(document.getElementById('" + chart_name + "')); " + chart_id_name + ".draw(" + data_name + ", {legend: 'bottom'}); "; break; }
		case EChartType::Bar: { output_data += "var " + chart_id_name + "=new google.visualization.BarChart(document.getElementById('" + chart_name + "')); " + chart_id_name + ".draw(" + data_name + ", {bar: {groupWidth: '95%'}}); "; break; }
		case EChartType::Column: { output_data += "var " + chart_id_name + "=new google.visualization.ColumnChart(document.getElementById('" + chart_name + "')); " + chart_id_name + ".draw(" + data_name + ", {bar: {groupWidth: '95%'}}); "; break; }
		case EChartType::Pie: { output_data += "var " + chart_id_name + "=new google.visualization.PieChart(document.getElementById('" + chart_name + "')); " + chart_id_name + ".draw(" + data_name + "); "; break; }
		case EChartType::Scatter: { output_data += "var " + chart_id_name + "=new google.visualization.ScatterChart(document.getElementById('" + chart_name + "')); " + chart_id_name + ".draw(" + data_name + "); "; break; }
		case EChartType::Timeline: { output_data += "var " + chart_id_name + "=new google.visualization.Timeline(document.getElementById('" + chart_name + "')); " + chart_id_name + ".draw(" + data_name + "); "; break; } //TODO implement datetime for timline
		}

		chart_id++;
	}
	output_graph += "</div><hr>";
}
