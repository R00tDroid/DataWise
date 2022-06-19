#include "AnalyticsMeshGeneration.h"
#include "DataWiseEditor.h"



namespace SphereGen
{
	const float X = .525731112119133606f;
	const float Z = .850650808352039932f;
	const float N = 0.f;

	static const TArray<FVector> vertices =
	{
	  {-X,N,Z}, {X,N,Z}, {-X,N,-Z}, {X,N,-Z},
	  {N,Z,X}, {N,Z,-X}, {N,-Z,X}, {N,-Z,-X},
	  {Z,X,N}, {-Z,X, N}, {Z,-X,N}, {-Z,-X, N}
	};

	static const TArray<FIntVector> faces =
	{
	  {0,4,1},{0,9,4},{9,5,4},{4,5,8},{4,8,1},
	  {8,10,1},{8,3,10},{5,3,8},{5,2,3},{2,7,3},
	  {7,10,3},{7,6,10},{7,11,6},{11,0,6},{0,1,6},
	  {6,1,10},{9,0,11},{9,11,2},{9,2,5},{7,2,11}
	};

	static FAnalyticsVisualizationMesh GenerateSphere0()
	{
		FAnalyticsVisualizationMesh mesh;
		for (FVector vertex : vertices)
		{
			mesh.Vertices.Add(FDynamicMeshVertex(vertex, FVector2D::ZeroVector, FColor::White));
		}

		for (FIntVector face : faces)
		{
			mesh.Indices.Add(face.X);
			mesh.Indices.Add(face.Y);
			mesh.Indices.Add(face.Z);
		}

		return mesh;
	}

	static FAnalyticsVisualizationMesh GenerateSphere(int level)
	{
		FAnalyticsVisualizationMesh mesh = GenerateSphere0();

		for (int i = 0; i < level; i++)
		{
			FAnalyticsVisualizationMesh new_mesh;

			for (int f = 0; f < mesh.Indices.Num(); f += 3)
			{
				FDynamicMeshVertex v0 = mesh.Vertices[mesh.Indices[f]];
				FDynamicMeshVertex v1 = mesh.Vertices[mesh.Indices[f + 1]];
				FDynamicMeshVertex v2 = mesh.Vertices[mesh.Indices[f + 2]];

				FDynamicMeshVertex v3 = v0; v3.Position = 0.5 * (v0.Position + v1.Position); v3.Position.Normalize();
				FDynamicMeshVertex v4 = v0; v4.Position = 0.5 * (v1.Position + v2.Position); v4.Position.Normalize();
				FDynamicMeshVertex v5 = v0; v5.Position = 0.5 * (v2.Position + v0.Position); v5.Position.Normalize();

				uint32 base_index = new_mesh.Vertices.Add(v0);
				new_mesh.Vertices.Add(v1);
				new_mesh.Vertices.Add(v2);
				new_mesh.Vertices.Add(v3);
				new_mesh.Vertices.Add(v4);
				new_mesh.Vertices.Add(v5);

				new_mesh.Indices.Add(base_index + 0); new_mesh.Indices.Add(base_index + 3); new_mesh.Indices.Add(base_index + 5);
				new_mesh.Indices.Add(base_index + 3); new_mesh.Indices.Add(base_index + 1); new_mesh.Indices.Add(base_index + 4);
				new_mesh.Indices.Add(base_index + 5); new_mesh.Indices.Add(base_index + 4); new_mesh.Indices.Add(base_index + 2);
				new_mesh.Indices.Add(base_index + 3); new_mesh.Indices.Add(base_index + 4); new_mesh.Indices.Add(base_index + 5);
			}

			mesh = new_mesh;
		}

		return mesh;
	}


	static TMap<int, FAnalyticsVisualizationMesh> Meshes;
}

FAnalyticsVisualizationMesh& MeshGeneration::GetSphere(int level)
{
	if (SphereGen::Meshes.Contains(level))
	{
		return SphereGen::Meshes[level];
	}
	else
	{
		FAnalyticsVisualizationMesh mesh = SphereGen::GenerateSphere(level);
		SphereGen::Meshes.Add(level, mesh);
	}

	return GetSphere(level);
}


namespace CubeGen
{
	static FAnalyticsVisualizationMesh Mesh;

	static void Generate()
	{
		Mesh.Vertices.Empty();
		Mesh.Indices.Empty();

		Mesh.Vertices.Add(FDynamicMeshVertex(FVector(0, 0, 0), FVector2D(0, 0), FColor::White));
		Mesh.Vertices.Add(FDynamicMeshVertex(FVector(1, 0, 0), FVector2D(0, 0), FColor::White));
		Mesh.Vertices.Add(FDynamicMeshVertex(FVector(1, 1, 0), FVector2D(0, 0), FColor::White));
		Mesh.Vertices.Add(FDynamicMeshVertex(FVector(0, 1, 0), FVector2D(0, 0), FColor::White));

		Mesh.Vertices.Add(FDynamicMeshVertex(FVector(0, 0, 1), FVector2D(0, 0), FColor::White));
		Mesh.Vertices.Add(FDynamicMeshVertex(FVector(1, 0, 1), FVector2D(0, 0), FColor::White));
		Mesh.Vertices.Add(FDynamicMeshVertex(FVector(1, 1, 1), FVector2D(0, 0), FColor::White));
		Mesh.Vertices.Add(FDynamicMeshVertex(FVector(0, 1, 1), FVector2D(0, 0), FColor::White));

		// Bottom
		Mesh.Indices.Add(0); Mesh.Indices.Add(1); Mesh.Indices.Add(2);
		Mesh.Indices.Add(2); Mesh.Indices.Add(3); Mesh.Indices.Add(0);

		// Top
		Mesh.Indices.Add(6); Mesh.Indices.Add(5); Mesh.Indices.Add(4);
		Mesh.Indices.Add(4); Mesh.Indices.Add(7); Mesh.Indices.Add(6);

		// Front
		Mesh.Indices.Add(0); Mesh.Indices.Add(4); Mesh.Indices.Add(1);
		Mesh.Indices.Add(4); Mesh.Indices.Add(5); Mesh.Indices.Add(1);

		// Back
		Mesh.Indices.Add(3); Mesh.Indices.Add(2); Mesh.Indices.Add(6);
		Mesh.Indices.Add(6); Mesh.Indices.Add(7); Mesh.Indices.Add(3);

		// Left
		Mesh.Indices.Add(0); Mesh.Indices.Add(3); Mesh.Indices.Add(7);
		Mesh.Indices.Add(7); Mesh.Indices.Add(4); Mesh.Indices.Add(0);

		// Right
		Mesh.Indices.Add(2); Mesh.Indices.Add(1); Mesh.Indices.Add(5);
		Mesh.Indices.Add(5); Mesh.Indices.Add(6); Mesh.Indices.Add(2);
	}

	static FAnalyticsVisualizationMesh& Get()
	{
		if (Mesh.Indices.Num() == 0) Generate();
		return Mesh;
	}	
}

FAnalyticsVisualizationMesh& MeshGeneration::GetCube()
{
	return CubeGen::Get();
}
