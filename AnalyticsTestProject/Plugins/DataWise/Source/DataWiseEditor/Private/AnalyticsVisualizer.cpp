#include "AnalyticsVisualizer.h"
#include "DataWiseEditor.h"
#include "SceneManagement.h"
#include "AnalyticsCompiler.h"
#include "AnalyticsWorker.h"
#include "EditorViewportClient.h"
#include "Editor.h"
#include "Kismet/KismetMathLibrary.h"
#include "LevelEditorViewport.h"
#include "SlateGameResources.h"

TArray<FAnalyticsVisualizationSet> UAnalyticsVisualizerComponent::datasets;
UAnalyticsVisualizerComponent::FOnVisualizationDataUpdated UAnalyticsVisualizerComponent::OnVisualizationDataUpdated;

UAnalyticsVisualizerComponent::UAnalyticsVisualizerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

	bAutoActivate = true;
	bTickInEditor = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.SetTickFunctionEnable(true);

	SetComponentTickEnabled(true);

	bWantsInitializeComponent = true;

	bUseEditorCompositing = true;
	SetGenerateOverlapEvents(false);

	bIgnoreStreamingManagerUpdate = true;

	started = false;

	static ConstructorHelpers::FObjectFinder<UMaterial> Material(TEXT("Material'/DataWise/Resources/VisualizerMaterial.VisualizerMaterial'"));

	if (Material.Object != nullptr)
	{
		VisualizerMaterial = static_cast<UMaterial*>(Material.Object);
	}
}

FBoxSphereBounds UAnalyticsVisualizerComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	const FVector BoxExtent(HALF_WORLD_MAX);
	return FBoxSphereBounds(FVector::ZeroVector, BoxExtent, BoxExtent.Size());
}

void UAnalyticsVisualizerComponent::UpdateData()
{
	Data.Empty();

	TArray<FAnalyticsVisualizationSet>& sets = UAnalyticsVisualizerComponent::GetDatasets();

	for(FAnalyticsVisualizationSet& set : sets)
	{
		if(set.Valid && set.Enabled) Data.Add(set.Output);
	}

	MarkRenderStateDirty();

	for (FEditorViewportClient* viewport : GEditor->AllViewportClients) 
	{
		viewport->Invalidate();
	}

	for(AActor* tag : tag_actors)
	{
		tag->Destroy();
	}

	tag_actors.Empty();

	for(FAnalyticsVisualizationData& set : Data)
	{
		for(FAnalyticsVisualizationTag& tag : set.Tags)
		{
			AStringTagActor* actor = GetWorld()->SpawnActor<AStringTagActor>(AStringTagActor::StaticClass());
			actor->Setup(tag.Text, tag.Location);
			tag_actors.Add(actor);
		}
	}	
}

void UAnalyticsVisualizerComponent::GetUsedMaterials(TArray<UMaterialInterface*> &OutMaterials, bool bGetDebugMaterials) const
{
	OutMaterials.Add(VisualizerMaterial);
}

FPrimitiveSceneProxy* UAnalyticsVisualizerComponent::CreateSceneProxy()
{
	if (GetScene() == nullptr) return nullptr;

	bTickInEditor = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.SetTickFunctionEnable(true);

	if (!started)
	{
		started = true;
		OnVisualizationDataUpdated.AddUObject(this, &UAnalyticsVisualizerComponent::UpdateData);
	}

	SetComponentTickEnabled(true);

	return new FAnalyticsVisualizerProxy(this);
}




void UAnalyticsVisualizerComponent::AddDataset(FAnalyticsVisualizationSet set)
{
	datasets.Add(set);
	OnVisualizationDataUpdated.Broadcast();

	UAnalyticsCacheTask* cache_task = UAnalyticsWorker::Get()->CreateTask<UAnalyticsCacheTask>();
	cache_task->captures_to_cache = set.Sessions;
	cache_task->notify = false;
	UAnalyticsWorker::Get()->AddTask(cache_task);

	UAnalyticsVisualizationTask* visualize_task = UAnalyticsWorker::Get()->CreateTask<UAnalyticsVisualizationTask>();
	visualize_task->dataset_to_compile = set;
	UAnalyticsWorker::Get()->AddTask(visualize_task);
}

void UAnalyticsVisualizerComponent::RemoveDataset(FAnalyticsVisualizationSet set)
{
	datasets.Remove(set);
	OnVisualizationDataUpdated.Broadcast();
}

TArray<FAnalyticsVisualizationSet>& UAnalyticsVisualizerComponent::GetDatasets()
{
	return datasets;
}

void UAnalyticsVisualizerComponent::NotifyDatasetUpdate()
{
	OnVisualizationDataUpdated.Broadcast();
}



FAnalyticsVisualizerProxy::FAnalyticsVisualizerProxy(UAnalyticsVisualizerComponent* component) : FPrimitiveSceneProxy(component, NAME_None)
{
	bWillEverBeLit = false;

	for(FAnalyticsVisualizationData& data : component->Data)
	{
		for (FAnalyticsVisualizationLine& line : data.Lines)
		{
			Lines.Add(line);
		}

		for (FAnalyticsVisualizationObject& mesh : data.Meshes)
		{
			Meshes.Add({ component->GetScene()->GetFeatureLevel(), mesh.Mesh.Vertices, mesh.Mesh.Indices, mesh.Color, mesh.Transformation });
		}
	}

	VisualizerMaterial = component->VisualizerMaterial;
}

FAnalyticsVisualizerProxy::~FAnalyticsVisualizerProxy()
{
	for (FRenderMesh& mesh : Meshes)
	{
		mesh.ReleaseResources();
	}
	Meshes.Empty();
}

void FAnalyticsVisualizerProxy::CreateRenderThreadResources()
{
	for (FRenderMesh& mesh : Meshes)
	{
		mesh.InitResources();
	}
}

SIZE_T FAnalyticsVisualizerProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

void FAnalyticsVisualizerProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];
			FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

			for(const FAnalyticsVisualizationLine& line : Lines)
			{
				PDI->DrawLine(line.Start, line.End, line.Color, 0, line.Thickness);
			}

			if (VisualizerMaterial != nullptr) 
			{
				for (const FRenderMesh& mesh : Meshes)
				{
					FMaterialRenderProxy* const material = VisualizerMaterial->GetRenderProxy(false);

					FMeshBatch& batch = Collector.AllocateMesh();
					batch.MaterialRenderProxy = material;
					batch.Elements[0].PrimitiveUniformBuffer = GetUniformBuffer();
					mesh.CreateBatch(batch);

					Collector.AddMesh(ViewIndex, batch);
				}
			}
		}
	}
}

uint32 FAnalyticsVisualizerProxy::GetMemoryFootprint() const
{
	return sizeof(*this) + GetAllocatedSize() + Lines.GetAllocatedSize() + Meshes.GetAllocatedSize();
}

FPrimitiveViewRelevance FAnalyticsVisualizerProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance ViewRelevance;
	ViewRelevance.bDrawRelevance = IsShown(View);
	ViewRelevance.bDynamicRelevance = Lines.Num() > 0 || Meshes.Num() > 0;
	ViewRelevance.bSeparateTranslucencyRelevance = ViewRelevance.bNormalTranslucencyRelevance = true;
	return ViewRelevance;
}


AStringTagActor::AStringTagActor(const FObjectInitializer& ObjectInitializer) :	Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	bIsEditorOnlyActor = true;
	bListedInSceneOutliner = false;

	widget = CreateDefaultSubobject<UWidgetComponent>("Widget");

	widget->SetWidgetSpace(EWidgetSpace::World);
	widget->SetVisibility(false);
	widget->SetDrawAtDesiredSize(true);
	widget->SetPivot(FVector2D(.5f, 1));

	TSharedPtr<SVerticalBox> holder;
	SAssignNew(holder, SVerticalBox);

	holder->AddSlot()[
		SAssignNew(widget_text, STextBlock)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 22))
	]
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	.AutoHeight();

	ConstructorHelpers::FObjectFinder<UTexture2D> TagPointer(TEXT("Texture2D'/DataWise/Resources/TagPointer.TagPointer'"));

	FSlateDynamicImageBrush* brush = new FSlateDynamicImageBrush(TagPointer.Object, FVector2D(77, 66), FName("tabActiveImage"));

	holder->AddSlot()[
		SNew(SBox)
		.WidthOverride(20)
		.HeightOverride(20)
		[
			SNew(SImage)
			.Image(brush)
		]
	]
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	.AutoHeight();

	widget->SetSlateWidget(holder);

	SetRootComponent(widget);

	Size = 1;
}

void AStringTagActor::Setup(FString Text, FVector Location)
{
	this->Location = Location;

	if (widget == nullptr) return;

	widget->SetRelativeLocation(Location);
	widget->SetVisibility(true);

	if (widget_text.IsValid()) widget_text->SetText(FText::FromString(Text));
}

void AStringTagActor::Tick(float DeltaTime)
{
	if (widget == nullptr) return;

	FLevelEditorViewportClient* Client = static_cast<FLevelEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
	if (Client)
	{
		FRotator camera_rotation = Client->GetViewRotation();
		
		widget->SetWorldRotation(FRotator(-camera_rotation.Pitch, camera_rotation.Yaw + 180, 0));

		if(widget_text.IsValid())
		{
			float distance = (Client->GetViewLocation() - Location).Size();

			float new_size = 1 + (distance - 500) / 500.0f;

			new_size = FMath::Max(1.0f, new_size);
			new_size = FMath::Min(10.0f, new_size);

			if (FMath::FloorToInt(new_size * 22) != FMath::FloorToInt(Size * 22))
			{
				Size = new_size;
				widget_text->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", FMath::FloorToInt(22 * Size)));
			}
		}
	}
}
