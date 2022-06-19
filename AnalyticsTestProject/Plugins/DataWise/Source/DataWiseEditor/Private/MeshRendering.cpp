#include "MeshRendering.h"
#include "DataWiseEditor.h"

FRenderMesh::FRenderMesh(ERHIFeatureLevel::Type InFeatureLevel, TArray<FDynamicMeshVertex> Vertices, TArray<uint32> Indices, FLinearColor Color, FMatrix Transformation) : Color(Color), Transformation(Transformation)
{
	FLinearColor vertex_color;
	for(FDynamicMeshVertex& vertex : Vertices)
	{
		VertexBuffer.Data.Add(Transformation.TransformFVector4(vertex.Position));

		UVBuffer.Data.Add(FVector2D::ZeroVector);
		NormalBuffer.Data.Add({ vertex.TangentX, vertex.TangentZ });

		vertex_color = vertex.Color.ReinterpretAsLinear();
		vertex_color *= Color;
		ColorBuffer.Data.Add(vertex_color.ToFColor(false));
	}

	IndexBuffer.Data = Indices;

	VertexFactory = new FRenderMeshVertexFactory(InFeatureLevel);
}

void FRenderMesh::ReleaseResources()
{
	VertexBuffer.ReleaseResource();
	ColorBuffer.ReleaseResource();
	UVBuffer.ReleaseResource();
	NormalBuffer.ReleaseResource();
	IndexBuffer.ReleaseResource();

	VertexFactory->ReleaseResource();
	delete VertexFactory;
	VertexFactory = nullptr;
}

void FRenderMesh::InitResources()
{
	check(IsInRenderingThread())

	VertexBuffer.UploadData();
	ColorBuffer.UploadData();
	UVBuffer.UploadData();
	NormalBuffer.UploadData();
	IndexBuffer.UploadData();

	FLocalVertexFactory::FDataType DataType;
	VertexBuffer.Bind(DataType);
	ColorBuffer.Bind(DataType);
	UVBuffer.Bind(DataType);
	NormalBuffer.Bind(DataType);

	VertexFactory->Init(DataType);
	VertexFactory->InitResource();
}

void FRenderMesh::CreateBatch(FMeshBatch& Batch) const
{
	FMeshBatchElement& BatchElement = Batch.Elements[0];
	BatchElement.IndexBuffer = &IndexBuffer;
	Batch.bWireframe = false;
	Batch.VertexFactory = VertexFactory;
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = IndexBuffer.Data.Num() / 3;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = VertexBuffer.Data.Num() - 1;
	Batch.Type = PT_TriangleList;
	Batch.DepthPriorityGroup = SDPG_World;
}


void FRenderMeshIndexBuffer::InitRHI()
{
	if (Data.Num() > 0) 
	{
		FRHIResourceCreateInfo CreateInfo;
		IndexBufferRHI = RHICreateIndexBuffer(Data.GetTypeSize(), GetDataSize(), BUF_Dynamic, CreateInfo);
	}
}

void FRenderMeshIndexBuffer::UploadData()
{
	check(IsInRenderingThread())

	InitResource();

	if (GetDataSize() > 0)
	{
		check(IndexBufferRHI.IsValid());

		void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, Data.Num(), RLM_WriteOnly);

		FMemory::Memcpy(Buffer, Data.GetData(), GetDataSize());

		RHIUnlockIndexBuffer(IndexBufferRHI);
	}
}

FRenderMeshVertexFactory::FRenderMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel) : FLocalVertexFactory(InFeatureLevel, "FRenderMeshVertexFactory")
{
}

void FRenderMeshVertexFactory::Init(FLocalVertexFactory::FDataType VertexStructure)
{
	check(IsInRenderingThread())
	SetData(VertexStructure);
}