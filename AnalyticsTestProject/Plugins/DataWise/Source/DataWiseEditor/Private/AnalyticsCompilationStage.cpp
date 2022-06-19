#include "AnalyticsCompilationStage.h"
#include "DataWiseEditor.h"
#include "HighResScreenshot.h"
#include "AnalyticsMapExport.h"
#include "AnalyticsChartExport.h"
#include "AnalyticsSheetExport.h"
#include "AnalyticsCompilerStaticData.h"
#include "HAL/PlatformFilemanager.h"
#include "Async/Async.h"
#include "Engine/Texture2D.h"

void UAnalyticsCompilationStage::ExportData(FString name)
{
	FString output_graph;
	FString output_data;

	int unique_id = 0;
	FString prefix = "";
	if(!name.IsEmpty()) prefix = name + "_";

	for(UAnalyticsExportableData* exporter : exporters)
	{
		exporter->ExportData(prefix,unique_id, output_data, output_graph);
		exporter->RemoveFromRoot();
		exporter->ConditionalBeginDestroy();

		unique_id++;
	}

	exporters.Empty();

	FString data = CompilerStaticData::html_base;
	data = data.Replace(TEXT("<!-- graphs -->"), *output_graph, ESearchCase::IgnoreCase);
	data = data.Replace(TEXT("<!-- data -->"), *output_data, ESearchCase::IgnoreCase);

	FString directory = FPaths::ProjectDir() + local_data_output;
	FString path = directory + prefix + GetClass()->GetName() + ".html";

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*directory);

	IFileHandle* file = PlatformFile.OpenWrite(*path);
	file->Write(reinterpret_cast<const uint8*>(TCHAR_TO_ANSI(*data)), data.Len());
	delete file;
}

void UAnalyticsCompilationStage::BeginDestroy()
{
	Super::BeginDestroy();

	for(UAnalyticsExportableData* exporter : exporters)
	{
		exporter->RemoveFromRoot();
		exporter->ConditionalBeginDestroy();
	}
	exporters.Empty();
}

UAnalyticsMap* UAnalyticsCompilationStage::CreateMap(FString Name, int Width, int Height, UTexture2D* Background, FVector2D WorldSize, FVector2D WorldCenter)
{
	UAnalyticsMap* map = NewObject<UAnalyticsMap>();
	map->Setup(Width, Height, Background, WorldSize, WorldCenter);
	map->ExportName = Name;
	map->AddToRoot();
	exporters.Add(map);
	return map;
}

UAnalyticsTable* UAnalyticsCompilationStage::CreateTable(FString Name, TArray<EChartType> ChartTypes, TMap<FString, ETableDataType> Columns)
{
	UAnalyticsTable* table = NewObject<UAnalyticsTable>();
	table->Setup(ChartTypes, Columns);
	table->ExportName = Name;
	table->AddToRoot();
	exporters.Add(table);
	return table;
}

UAnalyticsSheet* UAnalyticsCompilationStage::CreateSheet(FString Name, TMap<FString, ESheetDataType> Columns, FString Seperator)
{
	UAnalyticsSheet* sheet = NewObject<UAnalyticsSheet>();
	sheet->Setup(Seperator, Columns);
	sheet->ExportName = Name;
	sheet->AddToRoot();
	exporters.Add(sheet);
	return sheet;
}


TArray<UAnalyticsPacket*> UAnalyticsCompilerContext::GetAllPackets()
{
	return Packets;
}

TArray<UAnalyticsPacket*> UAnalyticsCompilerContext::GetPacketsInTimeFrame(int32 start, int32 end)
{
	TArray<UAnalyticsPacket*> result;
	for(UAnalyticsPacket* packet : Packets)
	{
		if ((int32)packet->Time >= start && (int32)packet->Time <= end) result.Add(packet);
	}
	return result;
}

TArray<UAnalyticsPacket*> UAnalyticsCompilerContext::GetPacketsOfType(TSubclassOf<UAnalyticsPacket> type)
{
	TArray<UAnalyticsPacket*> result;
	for (UAnalyticsPacket* packet : Packets)
	{
		if (packet->GetClass()->IsChildOf(type)) result.Add(packet);
	}
	return result;
}

BitmapRenderer::BitmapRenderer(int w, int h)
{
	size = FIntPoint(w, h);
	data.InsertZeroed(0, w * h);

	Clear(FColor::Black);
}

void BitmapRenderer::Clear(FColor color)
{
	uint32 index;
	for (int32 y = 0; y < size.Y; y++)
	{
		for (int32 x = 0; x < size.X; x++)
		{
			index = LocalToIndex({ x, y });
			data[index] = color;
		}
	}
}

void BitmapRenderer::DrawPixel(FIntPoint position , FColor color)
{
	if (color.A == 0) return;
	uint32 index = LocalToIndex(position);
	if (!data.IsValidIndex(index) || position.X < 0 || position.X >= size.X || position.Y < 0 || position.Y >= size.Y) return;

	if (color.A == 255) 
	{
		data[index] = color;
	} else
	{

		FLinearColor bg = FLinearColor(data[index].R / 255.0f, data[index].G / 255.0f, data[index].B / 255.0f, data[index].A);
		FLinearColor fg = FLinearColor(color.R / 255.0f, color.G / 255.0f, color.B / 255.0f, color.A / 255.0f);
		FLinearColor r;
		r.R = (fg.R * fg.A) + (bg.R * (1.0f - fg.A));
		r.G = (fg.G * fg.A) + (bg.G * (1.0f - fg.A));
		r.B = (fg.B * fg.A) + (bg.B * (1.0f - fg.A));
		r.A = fg.A;

		data[index] = FColor(r.R * 255, r.G * 255, r.B * 255, r.A * 255);
	}
}

void BitmapRenderer::DrawTriangle(FIntPoint a, FIntPoint b, FIntPoint c, FColor color)
{
	DrawTriangle(a, b, c, color, color, color);
}

void BitmapRenderer::DrawTriangle(FIntPoint a, FIntPoint b, FIntPoint c, FColor ca, FColor cb, FColor cc)
{
	FIntPoint min = a;
	FIntPoint max = a;
	FVector2D middle = a + b + c;
	middle /= 3;

	min.X = FMath::Min(FMath::Min(min.X, b.X), c.X);
	min.Y = FMath::Min(FMath::Min(min.Y, b.Y), c.Y);
	max.X = FMath::Max(FMath::Max(max.X, b.X), c.X);
	max.Y = FMath::Max(FMath::Max(max.Y, b.Y), c.Y);

	for (int y = min.Y; y < max.Y; y++)
	{
		for (int x = min.X; x < max.X; x++)
		{
			FVector2D p1 = a;
			FVector2D p2 = b;
			FVector2D p3 = c;

			FVector2D f((float)x, (float)y);

			FVector2D f1 = p1 - f;
			FVector2D f2 = p2 - f;
			FVector2D f3 = p3 - f;

			float alpha = FVector2D::CrossProduct(p1 - p2, p1 - p3);
			float alpha1 = FVector2D::CrossProduct(f2, f3) / alpha;
			float alpha2 = FVector2D::CrossProduct(f3, f1) / alpha;
			float alpha3 = FVector2D::CrossProduct(f1, f2) / alpha;

			if (alpha1 >= 0 && alpha2 >= 0 && alpha3 >= 0) {
				FColor color = (FLinearColor(ca) * alpha1 + FLinearColor(cb) * alpha2 + FLinearColor(cc) * alpha3).ToFColor(false);
				DrawPixel({ x, y }, color);
			}			
		}
	}
}

void BitmapRenderer::DrawLine(FIntPoint a, FIntPoint b, FColor color)
{
	bool steep = (FMath::Abs(b.Y - a.Y) > FMath::Abs(b.X - a.X));
	if (steep)
	{
		int32 tmp = a.X;
		a.X = a.Y;
		a.Y = tmp;

		tmp = b.X;
		b.X = b.Y;
		b.Y = tmp;
	}

	if (a.X > b.X)
	{
		int32 tmp = a.X;
		a.X = b.X;
		b.X = tmp;

		tmp = a.Y;
		a.Y = b.Y;
		b.Y = tmp;
	}

	float x_delta = b.X - a.X;
	float y_delta = FMath::Abs(b.Y - a.Y);

	float error = x_delta / 2.0f;
	int y_step = (a.Y < b.Y) ? 1 : -1;

	int y = a.Y;

	for (int x = a.X; x < b.X; x++)
	{
		if (steep)
		{
			DrawPixel(FIntPoint(y, x), color);
		}
		else
		{
			DrawPixel(FIntPoint(x, y), color);
		}

		error -= y_delta;
		if (error < 0)
		{
			error += x_delta;
			y += y_step;
		}
	}
}

void BitmapRenderer::DrawRectangle(FIntPoint pos, FIntPoint size, FColor color)
{
	for (int y = pos.Y; y < pos.Y + size.Y; y++)
	{
		for (int x = pos.X; x < pos.X + size.X; x++) {
			DrawPixel({ x, y }, color);
		}
	
	}
}

void BitmapRenderer::DrawRectangle(FIntPoint a, FIntPoint b, FIntPoint c, FIntPoint d, FColor color)
{
	DrawRectangle(a, b, c, d, color, color, color, color);
}

void BitmapRenderer::DrawRectangle(FIntPoint a, FIntPoint b, FIntPoint c, FIntPoint d, FColor ca, FColor cb, FColor cc, FColor cd)
{
	DrawTriangle(a, b, c, ca, cb, cc);
	DrawTriangle(c, d, a, cc, cd, ca);
}

void BitmapRenderer::DrawImage(FIntPoint pos, FIntPoint size, UTexture2D* texture)
{
	if (texture == nullptr) return;

	TArray<FColor> bitmap = BitmapFromTexture(texture);

	// Draw image to bitmap from pixel data
	for (int y = 0; y < size.Y; y++)
	{
		for (int x = 0; x < size.X; x++)
		{
			FVector2D uv = { x / (float)size.X , y / (float)size.Y };
			FVector2D bitmap_point = uv * FVector2D(texture->GetSizeX(), texture->GetSizeY());
			uint32 index = FMath::FloorToInt(bitmap_point.X) + FMath::FloorToInt(bitmap_point.Y) * texture->GetSizeX();
			DrawPixel(pos + FIntPoint(x, y), bitmap[index]);
		}
	}
}

uint32 BitmapRenderer::LocalToIndex(FIntPoint point)
{
	return point.X + point.Y * size.X;
}

FIntPoint BitmapRenderer::IndexToLocal(uint32)
{
	return{ 0, 0 };
}

TArray<FColor> threaded_bitmap;
FThreadSafeBool bitmap_loading = false;

TArray<FColor> BitmapRenderer::BitmapFromTexture(UTexture2D* texture)
{

	bitmap_loading = true;
	threaded_bitmap.Empty();

	AsyncTask(ENamedThreads::GameThread, [texture]()
	{
		// Request image type
		TextureCompressionSettings compression_settings = texture->CompressionSettings;
		TextureMipGenSettings mipmap_settings = texture->MipGenSettings;
		bool srgb_settings = texture->SRGB;

		texture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
		texture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
		texture->SRGB = false;
		texture->UpdateResource();

		const FColor* FormatedImageData = static_cast<const FColor*>(texture->PlatformData->Mips[0].BulkData.LockReadOnly());

		for (int32 Y = 0; Y < texture->GetSizeY(); Y++)
		{
			for (int32 X = 0; X < texture->GetSizeX(); X++)
			{
				FColor PixelColor = FormatedImageData[Y * texture->GetSizeX() + X];
				threaded_bitmap.Add(PixelColor);
			}
		}

		texture->PlatformData->Mips[0].BulkData.Unlock();

		// Restore image type
		texture->CompressionSettings = compression_settings;
		texture->MipGenSettings = mipmap_settings;
		texture->SRGB = srgb_settings;
		texture->UpdateResource();

		bitmap_loading = false;
	});

	while (bitmap_loading)
	{
		FPlatformProcess::Sleep(.1f);
	}

	TArray<FColor> bitmap = threaded_bitmap;
	threaded_bitmap.Empty();
	return bitmap;
}

void BitmapRenderer::Export(FString path)
{
	FString ResultPath;
	FHighResScreenshotConfig& HighResScreenshotConfig = GetHighResScreenshotConfig();
	bool bSaved = HighResScreenshotConfig.SaveImage(path, data, size, &ResultPath);
}
