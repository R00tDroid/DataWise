#include "AnalyticsVisualizationStage.h"
#include "DataWiseEditor.h"
#include "AnalyticsMeshGeneration.h"

void UAnalyticsVisualizableLine::ExportData(FAnalyticsVisualizationData& data)
{
	data.Lines.Add(FAnalyticsVisualizationLine(Start, End, Color, Thickness));
}


void UAnalyticsVisualizableSphere::ExportData(FAnalyticsVisualizationData& data)
{
	FAnalyticsVisualizationObject mesh(MeshGeneration::GetSphere(Level));
	mesh.Color = Color;
	mesh.Transformation = FScaleMatrix(Radius) * FTranslationMatrix(Location);
	data.Meshes.Add(mesh);
}

void UAnalyticsVisualizableCube::ExportData(FAnalyticsVisualizationData& data)
{
	FAnalyticsVisualizationObject mesh(MeshGeneration::GetCube());
	mesh.Color = Color;
	mesh.Transformation = FScaleMatrix(Size) * FTranslationMatrix(Location);
	data.Meshes.Add(mesh);
}

void UAnalyticsVisualizableTrail::AddPoint(FVector Point)
{
	Points.Add(Point);
}

void UAnalyticsVisualizableTrail::ExportData(FAnalyticsVisualizationData& data)
{
	for (int i = 1; i < Points.Num(); i++)
	{
		data.Lines.Add(FAnalyticsVisualizationLine(Points[i - 1], Points[i], Color, Thickness));
	}

	for (int i = 0; i < Points.Num(); i++)
	{
		FAnalyticsVisualizationObject mesh(MeshGeneration::GetSphere(0));
		mesh.Color = FLinearColor::Yellow;
		mesh.Transformation = FScaleMatrix(Thickness) * FTranslationMatrix(Points[i]);

		if (i == 0) { mesh.Color = FLinearColor::Blue; mesh.Transformation = FScaleMatrix(Thickness * 3) * FTranslationMatrix(Points[i]);}
		if (i == Points.Num() - 1) { mesh.Color = FLinearColor::Red; mesh.Transformation = FScaleMatrix(Thickness * 3) * FTranslationMatrix(Points[i]); }

		data.Meshes.Add(mesh);
	}
}

void UAnalyticsVisualizableTag::ExportData(FAnalyticsVisualizationData& data)
{
	data.Tags.Add(FAnalyticsVisualizationTag(Text, Location));
}



void UAnalyticsVisualizableAreaDensity::WriteDensity(FVector Location, float Density)
{
	Location /= DensityResolution;
	Location = FVector(FMath::FloorToFloat(Location.X), FMath::FloorToFloat(Location.Y), FMath::FloorToFloat(Location.Z));
	Location *= DensityResolution;

	if (DensityData.Contains(Location))
	{
		DensityData[Location] = Density;
	}
	else
	{
		DensityData.Add(Location, Density);
	}
}

void UAnalyticsVisualizableAreaDensity::AddDensity(FVector Location, float Density)
{
	Location /= DensityResolution;
	Location = FVector(FMath::FloorToFloat(Location.X), FMath::FloorToFloat(Location.Y), FMath::FloorToFloat(Location.Z));
	Location *= DensityResolution;

	if (DensityData.Contains(Location))
	{
		DensityData[Location] += Density;
	}
	else
	{
		DensityData.Add(Location, Density);
	}
}

void UAnalyticsVisualizableAreaDensity::ExportData(FAnalyticsVisualizationData& data)
{
	float density_min = 0;
	float density_max = 0;

	for (auto tile : DensityData)
	{
		if (density_min == 0 && density_max == 0)
		{
			density_min = tile.Value;
			density_max = tile.Value;
		}

		density_min = FMath::Min(density_min, tile.Value);
		density_max = FMath::Max(density_max, tile.Value);
	}


	for (auto tile : DensityData)
	{
		float progress = 1.0f;
		if (density_min != density_max) { progress = (tile.Value - density_min) / (density_max - density_min); }

		FColor color;
		if (progress <= .5f)
		{
			color = FColor(0, progress * 2 * 255, (1.0f - progress * 2.0f) * 255, 200);
		}
		else
		{
			color = FColor((progress - 0.5f) * 2 * 255, (1.0f - (progress - 0.5f) * 2) * 255, 0, 200);
		}

		color.A = 150;

		FAnalyticsVisualizationObject object(MeshGeneration::GetCube());
		object.Transformation = FScaleMatrix(DensityResolution) * FTranslationMatrix(tile.Key);
		object.Color = color;
		data.Meshes.Add(object);
	}
}



void UAnalyticsVisualizationStage::ExportData(FAnalyticsVisualizationData& data)
{
	for (UAnalyticsVisualizableData* visualizer : visualizers) {
		visualizer->ExportData(data);
	}

	for (UAnalyticsVisualizableData* visualizer : visualizers)
	{
		visualizer->RemoveFromRoot();
		visualizer->ConditionalBeginDestroy();
	}
	visualizers.Empty();
}

void UAnalyticsVisualizationStage::CreateLine(FVector Start, FVector End, FLinearColor Color, float Thickness)
{
	UAnalyticsVisualizableLine* line = NewObject<UAnalyticsVisualizableLine>();
	line->Start = Start;
	line->End = End;
	line->Color = Color;
	line->Thickness = Thickness;
	line->AddToRoot();
	visualizers.Add(line);
}

void UAnalyticsVisualizationStage::CreateSphere(FVector Location, float Radius, FLinearColor Color, int Level)
{
	UAnalyticsVisualizableSphere* sphere = NewObject<UAnalyticsVisualizableSphere>();
	sphere->AddToRoot();
	sphere->Location = Location;
	sphere->Radius = Radius;
	sphere->Color = Color;
	sphere->Level = Level;
	visualizers.Add(sphere);
}

void UAnalyticsVisualizationStage::CreateCube(FVector Location, FVector Size, FLinearColor Color)
{
	UAnalyticsVisualizableCube* cube = NewObject<UAnalyticsVisualizableCube>();
	cube->AddToRoot();
	cube->Location = Location;
	cube->Size = Size;
	cube->Color = Color;
	visualizers.Add(cube);
}

UAnalyticsVisualizableTrail* UAnalyticsVisualizationStage::CreateTrail(FLinearColor Color, float Thickness)
{
	UAnalyticsVisualizableTrail* trail = NewObject<UAnalyticsVisualizableTrail>();
	trail->AddToRoot();
	trail->Color = Color;
	trail->Thickness = Thickness;
	visualizers.Add(trail);
	return trail;
}

void UAnalyticsVisualizationStage::CreateTag(FString Text, FVector Location)
{
	UAnalyticsVisualizableTag* tag = NewObject<UAnalyticsVisualizableTag>();
	tag->AddToRoot();
	tag->Text = Text;
	tag->Location = Location;
	visualizers.Add(tag);
}

UAnalyticsVisualizableAreaDensity* UAnalyticsVisualizationStage::CreateDensityMap(FVector Resolution)
{
	UAnalyticsVisualizableAreaDensity* map = NewObject<UAnalyticsVisualizableAreaDensity>();
	map->AddToRoot();
	map->DensityResolution = Resolution;
	visualizers.Add(map);
	return map;
}


