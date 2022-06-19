#pragma once
#include "PrimitiveSceneProxy.h"
#include "Components/PrimitiveComponent.h"
#include "Engine.h"
#include "MeshRendering.h"
#include "WidgetComponent.h"
#include "AnalyticsVisualizer.generated.h"


struct FAnalyticsVisualizationMesh
{
	FAnalyticsVisualizationMesh(){}

	TArray<FDynamicMeshVertex> Vertices;
	TArray<uint32> Indices;
};

struct FAnalyticsVisualizationObject
{
	FAnalyticsVisualizationObject(FAnalyticsVisualizationMesh& Mesh) : Mesh(Mesh) {}

	FAnalyticsVisualizationMesh Mesh;
	FMatrix Transformation = FMatrix::Identity;
	FLinearColor Color = FLinearColor::White;
};


struct FAnalyticsVisualizationLine
{
	FAnalyticsVisualizationLine(FVector Start, FVector End, FLinearColor Color, float Thickness) : Start(Start), End(End), Color(Color), Thickness(Thickness){}

	FVector Start;
	FVector End;
	FLinearColor Color;
	float Thickness;
};

struct FAnalyticsVisualizationTag
{
	FAnalyticsVisualizationTag(FString Text,FVector Location) : Text(Text), Location(Location) {}
	FString Text;
	FVector Location;
};

struct FAnalyticsVisualizationData
{
	TArray<FAnalyticsVisualizationLine> Lines;
	TArray<FAnalyticsVisualizationObject> Meshes;
	TArray<FAnalyticsVisualizationTag> Tags;
};


struct FAnalyticsVisualizationSet
{
	TArray<FAnalyticsCaptureInfo> Sessions;
	TArray<UClass*> Stages;
	FAnalyticsVisualizationData Output;

	bool Valid = false;
	float Progress = 0.0f;
	bool Enabled = true;
	FString FilterString;


	bool operator == (const FAnalyticsVisualizationSet& other)
	{
		return Sessions == other.Sessions && Stages == other.Stages;
	}
};

inline bool operator == (const FAnalyticsVisualizationSet& a, const FAnalyticsVisualizationSet& b)
{
	return a.Sessions == b.Sessions && a.Stages == b.Stages;
}


UCLASS()
class UAnalyticsVisualizerComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UAnalyticsVisualizerComponent(const FObjectInitializer& ObjectInitializer);

	FPrimitiveSceneProxy* CreateSceneProxy() override;

	FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

	void GetUsedMaterials(TArray<UMaterialInterface*> &OutMaterials, bool bGetDebugMaterials) const override;

	void UpdateData();

	TArray<FAnalyticsVisualizationData> Data;
	UMaterial* VisualizerMaterial;

	static void AddDataset(FAnalyticsVisualizationSet);
	static void RemoveDataset(FAnalyticsVisualizationSet);
	static TArray<FAnalyticsVisualizationSet>& GetDatasets();
	static void NotifyDatasetUpdate();

	DECLARE_MULTICAST_DELEGATE(FOnVisualizationDataUpdated);
	static FOnVisualizationDataUpdated OnVisualizationDataUpdated;

private:
	static TArray<FAnalyticsVisualizationSet> datasets;

	TArray<AActor*> tag_actors;

	bool started = false;
};


class DATAWISEEDITOR_API FAnalyticsVisualizerProxy : public FPrimitiveSceneProxy
{
public:

	FAnalyticsVisualizerProxy(UAnalyticsVisualizerComponent*);
	~FAnalyticsVisualizerProxy();

	void CreateRenderThreadResources() override;

	SIZE_T GetTypeHash() const override;

	void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	
	uint32 GetMemoryFootprint() const override;
	FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

private:

	TArray<FAnalyticsVisualizationLine> Lines;
	TArray<FRenderMesh> Meshes;

	UMaterial* VisualizerMaterial;
};


UCLASS()
class DATAWISEEDITOR_API AStringTagActor : public AActor
{
	GENERATED_BODY()
public:
	AStringTagActor(const FObjectInitializer& ObjectInitializer);

	void Setup(FString Text, FVector Location);

	void Tick(float DeltaTime) override;

	bool IsSelectable() const override { return false; }

	bool ShouldTickIfViewportsOnly() const override	{ return true; }

private:
	
	UWidgetComponent* widget;
	TSharedPtr<STextBlock> widget_text;
	FVector Location;

	float Size;
};