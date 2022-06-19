#include "AnalyticsSheetExport.h"
#include "DataWiseEditor.h"
#include "PlatformFilemanager.h"


void UAnalyticsSheet::Setup(FString Seperator, TMap<FString, ESheetDataType> Columns)
{
	seperator_type = Seperator;

	Columns.GetKeys(column_names);
	for (FString column : column_names)
	{
		column_types.Add(Columns[column]);
	}
}

void UAnalyticsSheet::AddRow(TArray<FString> ColumnData)
{
	if (ColumnData.Num() != column_types.Num()) { UE_LOG(AnalyticsLogEditor, Error, TEXT("Added column data does not match signature")); return; }
	rows.Add(ColumnData);
}

void UAnalyticsSheet::ExportData(FString prefix, uint32, FString&, FString&)
{
	

	FString data = "";

	for (FString column : column_names) 
	{
		data += column + seperator_type;
	} 
	data += "\n";

	for (TArray<FString> row : rows)
	{
		for (int id = 0; id < row.Num(); id++)
		{
			switch (column_types[id])
			{
			case ESheetDataType::String: { data += row[id] + seperator_type; break; }
			case ESheetDataType::Number: { data += row[id].Replace(TEXT("."), TEXT(",")) + seperator_type; break; }
			case ESheetDataType::Boolean: { data += row[id] + seperator_type; break; }
			case ESheetDataType::Time:
			{
				int seconds = FCString::Atoi(*row[id]);
				int minutes = FMath::FloorToInt(seconds / 60.0f);
				int hours = FMath::FloorToInt(minutes / 60.0f);

				seconds -= minutes * 60;
				minutes -= hours * 60;

				data += FString::FromInt(hours) + ":" + FString::FromInt(minutes) + ":" + FString::FromInt(seconds) + seperator_type; break;
			}
			}
		}
		data += "\n";
	}

	FString directory = FPaths::ProjectDir() + local_data_output;
	FString filename = prefix + ExportName + ".csv";
	FString path = directory + filename;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*directory);

	IFileHandle* file = PlatformFile.OpenWrite(*path);
	file->Write(reinterpret_cast<const uint8*>(TCHAR_TO_ANSI(*data)), data.Len());
	delete file;
}
