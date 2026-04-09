#include "ASDFBakeActor.h"

#include "EngineUtils.h"
#include "MeshAttributes.h"
#include "UMassSDFSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAABBTree3.h"
#include "Async/ParallelFor.h"
#include "Spatial/FastWinding.h"

ASDFBakeActor::ASDFBakeActor()
{
	BakeBoundsVisualizer = CreateDefaultSubobject<UBoxComponent>(TEXT("BakeBounds"));
	RootComponent = BakeBoundsVisualizer;
    
	// Default size for a small level chunk
	BakeBoundsVisualizer->SetBoxExtent(FVector(500.f, 500.f, 500.f));
	
#if WITH_EDITOR
	bIsEditorOnlyActor = true;
#endif
}

void ASDFBakeActor::BeginPlay()
{
	Super::BeginPlay();
	if (TargetAsset)
	{
		if (UMassSDFSubsystem* SDFSubsystem = GetWorld()->GetSubsystem<UMassSDFSubsystem>())
		{
			SDFSubsystem->RegisterSDFAsset(TargetAsset);
		}
	} else
	{
        UE_LOG(LogTemp, Error, TEXT("ASDFBakeActor: No Target Asset assigned!"));
	}
}

void ASDFBakeActor::TriggerBake() const
{
    if (!TargetAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("ASDFBakeActor: SDF Bake failed: No Target Asset assigned!"));
        return;
    }

    // Define the world area to voxelize
    FBox BakingBounds = BakeBoundsVisualizer->Bounds.GetBox();

    // Start the process
    BakeSignedDistanceField(VoxelSize, BakingBounds);
}

void ASDFBakeActor::ExtractMeshesFromActor(AActor* RootActor, TArray<UStaticMeshComponent*>& OutMeshComps)
{
    if (!RootActor) return;

    // Get all Mesh Components owned directly by this actor (Static, Instanced, and Hierarchical)
    TArray<UStaticMeshComponent*> LocalMeshes;
    RootActor->GetComponents<UStaticMeshComponent>(LocalMeshes);
    OutMeshComps.Append(LocalMeshes);

    // Look for Child Actor Components for nested Blueprints
    TArray<UChildActorComponent*> ChildActorComps;
    RootActor->GetComponents<UChildActorComponent>(ChildActorComps);

    for (UChildActorComponent* CAC : ChildActorComps)
    {
	    if (AActor* ChildActor = CAC->GetChildActor())
        {
            ExtractMeshesFromActor(ChildActor, OutMeshComps);
        }
    }
}
void ASDFBakeActor::BakeSignedDistanceField(float InVoxelSize, FBox InBounds) const
{
#if WITH_EDITOR
    using namespace UE::Geometry;
    
    if (!TargetAsset) return;

    // SETUP GRID
    TargetAsset->VoxelSize = InVoxelSize;
    TargetAsset->WorldBounds = InBounds;
    FVector Size = TargetAsset->WorldBounds.GetSize();
    TargetAsset->GridDims = FIntVector(
        FMath::CeilToInt(Size.X / TargetAsset->VoxelSize),
        FMath::CeilToInt(Size.Y / TargetAsset->VoxelSize),
        FMath::CeilToInt(Size.Z / TargetAsset->VoxelSize)
    );

    FDynamicMesh3 CombinedMesh;
    
    // GEOMETRY MERGING
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* CurrentActor = *It;

        // Avoid Double-Counting. 
        // Only start extraction from "Top Level" actors. 
        // If an actor has a parent, it will be reached via recursion from the root.
        if (CurrentActor == this || CurrentActor->GetParentActor() != nullptr) continue;

        TArray<UStaticMeshComponent*> AllMeshes;
        ExtractMeshesFromActor(CurrentActor, AllMeshes);

        for (UStaticMeshComponent* MeshComp : AllMeshes)
        {
            UStaticMesh* StaticMesh = MeshComp->GetStaticMesh();
            if (!StaticMesh) continue;

            const FMeshDescription* MeshDesc = StaticMesh->GetMeshDescription(0);
            if (!MeshDesc) continue;

            TVertexAttributesConstRef<FVector3f> VertexPositions = MeshDesc->VertexAttributes().GetAttributesRef<FVector3f>(MeshAttribute::Vertex::Position);

            TArray<FTransform> InstanceTransforms;
            if (UInstancedStaticMeshComponent* ISM = Cast<UInstancedStaticMeshComponent>(MeshComp))
            {
                for (int32 i = 0; i < ISM->GetInstanceCount(); ++i)
                {
                    FTransform IT;
                    ISM->GetInstanceTransform(i, IT, true);
                    InstanceTransforms.Add(IT);
                }
            }
            else
            {
                InstanceTransforms.Add(MeshComp->GetComponentTransform());
            }

            for (const FTransform& CurrentTransform : InstanceTransforms)
            {
                // Handle Winding Order for Negative Scales
                // If the transform mirrors the mesh, we must flip the triangle indices.
                FVector S = CurrentTransform.GetScale3D();
                bool bReverseWinding = (S.X * S.Y * S.Z) < 0.0f;

                TMap<FVertexID, int32> VMap;
                for (const FVertexID VertexID : MeshDesc->Vertices().GetElementIDs())
                {
                    FVector3d WorldPos = CurrentTransform.TransformPosition((FVector3d)VertexPositions[VertexID]);
                    VMap.Add(VertexID, CombinedMesh.AppendVertex(WorldPos));
                }

                for (const FTriangleID TriID : MeshDesc->Triangles().GetElementIDs())
                {
                    TArrayView<const FVertexID> V = MeshDesc->GetTriangleVertices(TriID);
                    
                    if (bReverseWinding)
                    {
                        // Flip two indices to reverse the normal
                        CombinedMesh.AppendTriangle(VMap[V[0]], VMap[V[2]], VMap[V[1]]);
                    }
                    else
                    {
                        CombinedMesh.AppendTriangle(VMap[V[0]], VMap[V[1]], VMap[V[2]]);
                    }
                }
            }
        }
    }

    // BUILD ACCELERATION
    FDynamicMeshAABBTree3 SpatialTree(&CombinedMesh, true);
	TFastWindingTree WindingTree(&SpatialTree);

    int32 TotalVoxels = TargetAsset->GridDims.X * TargetAsset->GridDims.Y * TargetAsset->GridDims.Z;
    TargetAsset->DistanceData.Empty();
    TargetAsset->DistanceData.SetNumUninitialized(TotalVoxels);
    
    // PARALLEL VOXEL SAMPLING
    ParallelFor(TargetAsset->GridDims.Z, [&](int32 z)
    {
        for (int32 y = 0; y < TargetAsset->GridDims.Y; ++y)
        {
            for (int32 x = 0; x < TargetAsset->GridDims.X; ++x)
            {
                // FIX 3: Sample the CENTER of the voxel (+0.5f)
                FVector3d SamplePos = (FVector3d)TargetAsset->WorldBounds.Min + 
                                      FVector3d((x + 0.5) * TargetAsset->VoxelSize, 
                                                (y + 0.5) * TargetAsset->VoxelSize, 
                                                (z + 0.5) * TargetAsset->VoxelSize);
            
                double NearestDistSquared = 0.0;
                int32 NearestTriID = SpatialTree.FindNearestTriangle(SamplePos, NearestDistSquared);

                float FinalDistance = 10000.0f; 

                if (NearestTriID != IndexConstants::InvalidID)
                {
                    FinalDistance = (float)FMath::Sqrt(NearestDistSquared);
                	
                	double WindingValue = WindingTree.FastWindingNumber(SamplePos);
					if (WindingValue > 0.5)
					{
						FinalDistance *= -1.0f;
					}
                }

                int32 Index = x + (y * TargetAsset->GridDims.X) + (z * TargetAsset->GridDims.X * TargetAsset->GridDims.Y);
                TargetAsset->DistanceData[Index] = FinalDistance;
            }
        }
    });
    
    TargetAsset->Modify();
#else
	UE_LOG(LogTemp, Warning, TEXT("SDF Baking is not available in cooked builds."));
#endif
}

void ASDFBakeActor::VisualizeSDFSlice() const
{
    if (!TargetAsset || TargetAsset->DistanceData.Num() == 0) return;

    FlushPersistentDebugLines(GetWorld());

    int32 ZIndex = FMath::Clamp(
       FMath::FloorToInt((SliceVisualizationHeight - TargetAsset->WorldBounds.Min.Z) / TargetAsset->VoxelSize),
       0, TargetAsset->GridDims.Z - 1
    );

    for (int32 y = 0; y < TargetAsset->GridDims.Y; y += Stride)
    {
       for (int32 x = 0; x < TargetAsset->GridDims.X; x += Stride)
       {
          int32 Index = x + (y * TargetAsset->GridDims.X) + (ZIndex * TargetAsset->GridDims.X * TargetAsset->GridDims.Y);
          float Distance = TargetAsset->DistanceData[Index];
          FVector Pos = TargetAsset->IndexToWorld(Index);

          FColor DisplayColor;
          if (Distance < 0) 
          {
             DisplayColor = FColor::Red;
          } 
          else if (Distance < SliceVisualizationLerpDistance) 
          {
             float Lerp = FMath::Clamp(Distance / SliceVisualizationLerpDistance, 0.0f, 1.0f);
             DisplayColor = FColor(255 * (1.0f - Lerp), 255 * Lerp, 0); 
          } 
          else continue;

          // Draw the Voxel Point
          DrawDebugPoint(GetWorld(), Pos, DebugSize, DisplayColor, false, VisualizationTime);

          // Draw the Gradient Arrow (Only near/inside obstacles)
          if (bShowGradients && Distance < SliceVisualizationLerpDistance)
          {
             // Note: Ensure GetGradientAtWorldPosition is accessible here
             // You may need to pass TargetAsset or use a static helper
             FVector Grad = GetGradientAtWorldPosition(Pos); 
             
             if (!Grad.IsNearlyZero())
             {
                // Draw a line pointing away from the wall
                FVector ArrowEnd = Pos + (Grad * TargetAsset->VoxelSize);
                DrawDebugDirectionalArrow(GetWorld(), Pos, ArrowEnd, 5.0f, DisplayColor, false, VisualizationTime, 0, 2.0f);
             }
          }
       }
    }
}
void ASDFBakeActor::VisualizeSurface() const
{
	FlushPersistentDebugLines(GetWorld());
	
	for (int32 i = 0; i < TargetAsset->DistanceData.Num(); i += Stride)
	{
		float Dist = TargetAsset->DistanceData[i];

		// Only draw points that are exactly on the "edge" of a wall
		if (FMath::Abs(Dist) < (TargetAsset->VoxelSize * 1.f))
		{
			// Convert 1D Index back to 3D World Space (Helper function recommended)
			FVector VoxelCenter = TargetAsset->IndexToWorld(i); 
			FColor Color = (Dist < 0) ? FColor::Red : FColor::White;
			DrawDebugPoint(GetWorld(), VoxelCenter, DebugSize, Color, false, VisualizationTime);
		}
	}
}
