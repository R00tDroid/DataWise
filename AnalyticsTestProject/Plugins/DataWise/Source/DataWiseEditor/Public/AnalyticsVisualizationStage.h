#pragma once
#include "AnalyticsCompilationStage.h"
#include "AnalyticsVisualizer.h"
#include "AnalyticsVisualizationStage.generated.h"

UCLASS()
class UAnalyticsVisualizableData : public UObject
{
public:
	GENERATED_BODY()

	virtual void ExportData(FAnalyticsVisualizationData&) {};
};

UCLASS()
class UAnalyticsVisualizableLine : public UAnalyticsVisualizableData
{
public:
	GENERATED_BODY()

	void ExportData(FAnalyticsVisualizationData&) override;

	FVector Start;
	FVector End;
	FLinearColor Color;
	float Thickness;
};

UCLASS()
class UAnalyticsVisualizableSphere : public UAnalyticsVisualizableData
{
public:
	GENERATED_BODY()

	void ExportData(FAnalyticsVisualizationData&) override;

	FVector Location;
	float Radius;
	int Level;
	FLinearColor Color;
};

UCLASS()
class UAnalyticsVisualizableCube : public UAnalyticsVisualizableData
{
public:
	GENERATED_BODY()

	void ExportData(FAnalyticsVisualizationData&) override;

	FVector Location;
	FVector Size;
	FLinearColor Color;
};

UCLASS()
class UAnalyticsVisualizableTrail : public UAnalyticsVisualizableData
{
public:
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable)
	void AddPoint(FVector Point);

	void ExportData(FAnalyticsVisualizationData&) override;

	TArray<FVector> Points;
	FLinearColor Color;
	float Thickness;
};

UCLASS()
class UAnalyticsVisualizableTag : public UAnalyticsVisualizableData
{
public:
	GENERATED_BODY()

	void ExportData(FAnalyticsVisualizationData&) override;

	FString Text;
	FVector Location;
};

UCLASS()
class UAnalyticsVisualizableAreaDensity : public UAnalyticsVisualizableData
{
public:
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable)
	void WriteDensity(FVector Location, float Density);

	UFUNCTION(BlueprintCallable)
	void AddDensity(FVector Location, float Density);

	void ExportData(FAnalyticsVisualizationData&) override;

	FVector DensityResolution;

private:
	TMap<FVector, float> DensityData;
	
};


UCLASS(Blueprintable) 
class UAnalyticsVisualizationStage : public UObject {
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

	void ExportData(FAnalyticsVisualizationData&);

protected:
	UFUNCTION(BlueprintCallable)
	void CreateLine(FVector Start, FVector End, FLinearColor Color = FLinearColor::Red, float Thickness = 5);

	UFUNCTION(BlueprintCallable)
	void CreateSphere(FVector Location, float Radius, FLinearColor Color = FLinearColor::Red, int Level = 3);

	UFUNCTION(BlueprintCallable)
	void CreateCube(FVector Location, FVector Size, FLinearColor Color = FLinearColor::Red);

	UFUNCTION(BlueprintCallable)
	UAnalyticsVisualizableTrail* CreateTrail(FLinearColor Color = FLinearColor::Green, float Thickness = 5);

	UFUNCTION(BlueprintCallable)
	void CreateTag(FString Text, FVector Location);

	UFUNCTION(BlueprintCallable)
	UAnalyticsVisualizableAreaDensity* CreateDensityMap(FVector Resolution = FVector(200, 200, 200));

private:
	TArray<UAnalyticsVisualizableData*> visualizers;
};
