#pragma once
#include "DynamicMeshBuilder.h"

template<class T>
class FRenderMeshBuffer : public FVertexBuffer
{
public:
	TArray<T> Data;
	FShaderResourceViewRHIRef ShaderResource;

	void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(GetDataSize(), BUF_Static | BUF_ShaderResource, CreateInfo);

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
		if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
		{
			CreateSRV();
		}
#endif
	}

	virtual void UploadData()
	{
		check(IsInRenderingThread())

		InitResource();

		if (GetDataSize() > 0)
		{
			check(VertexBufferRHI.IsValid());

			void* Buffer = RHILockVertexBuffer(VertexBufferRHI, 0, GetDataSize(), RLM_WriteOnly);

			FMemory::Memcpy(Buffer, Data.GetData(), GetDataSize());

			RHIUnlockVertexBuffer(VertexBufferRHI);
		}
	}

	virtual int GetDataSize()
	{
		return Data.GetTypeSize() * Data.Num();
	}

	virtual void Bind(FLocalVertexFactory::FDataType& DataType) = 0;

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
	virtual void CreateSRV() = 0;
#endif
};


class FRenderMeshVertexBuffer : public FRenderMeshBuffer<FVector>
{
public:

	void Bind(FLocalVertexFactory::FDataType& DataType) override
	{
		DataType.PositionComponent = FVertexStreamComponent(this, 0, 12, VET_Float3);

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 19
		DataType.PositionComponentSRV = ShaderResource;
#endif
	}

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
	virtual void CreateSRV() override
	{
		ShaderResource = RHICreateShaderResourceView(VertexBufferRHI, 8, PF_A16B16G16R16);
	}
#endif
};


class FRenderMeshColorBuffer : public FRenderMeshBuffer<FColor>
{
public:

	void Bind(FLocalVertexFactory::FDataType& DataType) override
	{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 19
		DataType.ColorComponent = FVertexStreamComponent(this, 0, 4, EVertexElementType::VET_Color, EVertexStreamUsage::ManualFetch);
		DataType.ColorComponentsSRV = ShaderResource;
#else
		DataType.ColorComponent = FVertexStreamComponent(this, 0, 4, EVertexElementType::VET_Color);
#endif
	}

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
	virtual void CreateSRV() override
	{
		ShaderResource = RHICreateShaderResourceView(VertexBufferRHI, 4, PF_R8G8B8A8);
	}
#endif
};


class FRenderMeshUVBuffer : public FRenderMeshBuffer<FVector2D>
{
public:

	void Bind(FLocalVertexFactory::FDataType& DataType) override
	{
		DataType.TextureCoordinates.Empty();

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 19
		DataType.TextureCoordinates.Add(FVertexStreamComponent(this, 0, sizeof(FVector2D), VET_Float2, EVertexStreamUsage::ManualFetch));
#else
		DataType.TextureCoordinates.Add(FVertexStreamComponent(this, 0, sizeof(FVector2D), VET_Float2));
#endif

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 19
		DataType.TextureCoordinatesSRV = ShaderResource;
#endif
	}

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
	virtual void CreateSRV() override
	{
		ShaderResource = RHICreateShaderResourceView(VertexBufferRHI, 8, PF_G32R32F);
	}
#endif
};


struct FRenderMeshNormal
{
	FPackedNormal Tangent;
	FPackedNormal Normal;
};

class FRenderMeshNormalBuffer : public FRenderMeshBuffer<FRenderMeshNormal>
{
public:

	void Bind(FLocalVertexFactory::FDataType& DataType) override
	{
		uint32 TangentSizeInBytes = 0;
		uint32 TangentXOffset = 0;
		uint32 TangentZOffset = 0;
		EVertexElementType TangentElementType = VET_None;

		TangentElementType = VET_PackedNormal;
		TangentSizeInBytes = sizeof(FRenderMeshNormal);
		TangentXOffset = offsetof(FRenderMeshNormal, Tangent);
		TangentZOffset = offsetof(FRenderMeshNormal, Normal);

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 19
		DataType.TangentBasisComponents[0] = FVertexStreamComponent(this, TangentXOffset, TangentSizeInBytes, TangentElementType, EVertexStreamUsage::ManualFetch);
		DataType.TangentBasisComponents[1] = FVertexStreamComponent(this, TangentZOffset, TangentSizeInBytes, TangentElementType, EVertexStreamUsage::ManualFetch);
		DataType.TangentsSRV = ShaderResource;
#else
		DataType.TangentBasisComponents[0] = FVertexStreamComponent(this, TangentXOffset, TangentSizeInBytes, TangentElementType);
		DataType.TangentBasisComponents[1] = FVertexStreamComponent(this, TangentZOffset, TangentSizeInBytes, TangentElementType);
#endif
	}

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
	virtual void CreateSRV() override
	{
		ShaderResource = RHICreateShaderResourceView(VertexBufferRHI, 8, PF_A16B16G16R16);
	}
#endif
};


class FRenderMeshIndexBuffer : public FIndexBuffer
{
public:
	TArray<uint32> Data;

	void InitRHI() override;

	void UploadData();

	int GetDataSize()
	{
		return Data.GetTypeSize() * Data.Num();
	}
};


class FRenderMeshVertexFactory : public FLocalVertexFactory
{
public:

	FRenderMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel);

	void Init(FLocalVertexFactory::FDataType VertexStructure);
};


struct FRenderMesh
{
	FRenderMesh(ERHIFeatureLevel::Type InFeatureLevel, TArray<FDynamicMeshVertex> Vertices, TArray<uint32> Indices, FLinearColor Color, FMatrix Transformation);

	void InitResources();
	void ReleaseResources();

	void CreateBatch(FMeshBatch&) const;

	FLinearColor Color;
	FMatrix Transformation;

	FRenderMeshVertexBuffer VertexBuffer;
	FRenderMeshColorBuffer ColorBuffer;
	FRenderMeshUVBuffer UVBuffer;
	FRenderMeshNormalBuffer NormalBuffer;

	FRenderMeshIndexBuffer IndexBuffer;
	FRenderMeshVertexFactory* VertexFactory;
};