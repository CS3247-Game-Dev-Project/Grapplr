#include "AFlowFieldVolume.h"
#include "DrawDebugHelpers.h"
#include "UFlowFieldSubsystem.h"
#include "CS3247_Group2/Mass/Constants.h"
#include "Kismet/GameplayStatics.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"

#if WITH_EDITOR
#include "Editor.h"
#include "LevelEditorViewport.h"
#endif

AFlowFieldVolume::AFlowFieldVolume()
{
    BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("GridBounds"));
    RootComponent = BoxComponent;

    // Set default size
    BoxComponent->SetBoxExtent(FVector(500.0f, 500.0f, 250.0f));
    
    // Ensure it doesn't block the trace we're about to run
    BoxComponent->SetCollisionProfileName(TEXT("NoCollision")); 
    BoxComponent->SetCanEverAffectNavigation(false);
    
    // Create a local line batcher
    // Ensure it doesn't render in the actual game, only the editor
    MyLineBatcher = CreateDefaultSubobject<ULineBatchComponent>(TEXT("LocalLineBatcher"));
#if WITH_EDITOR
    MyLineBatcher->SetIsVisualizationComponent(true);
#endif
}

void AFlowFieldVolume::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    DrawFlowFieldDebug();
}

void AFlowFieldVolume::BeginPlay()
{
    Super::BeginPlay();
    if (auto* FlowFieldSubsystem = GetWorld()->GetSubsystem<UFlowFieldSubsystem>())
    {
        FlowFieldSubsystem->RegisterVolume(this);
    }
}

void AFlowFieldVolume::BakeFlowField() const
{
    if (!TargetAsset || !BoxComponent)
    {
		UE_LOG(LogTemp, Warning, TEXT("AFlowFieldVolume: No target asset found, unable to bake! Please create a UFlowDataAsset, and add it into the AFlowFieldVolume!"));
        return;
    }

    // Get the actual scaled extent and world center from the BoxComponent
    FVector Extent = BoxComponent->GetScaledBoxExtent();
    FVector Origin = BoxComponent->GetComponentLocation();
    int32 GridX = FMath::FloorToInt((Extent.X * 2) / CellSize);
    int32 GridY = FMath::FloorToInt((Extent.Y * 2) / CellSize);
    FVector RawMin = Origin - Extent;
    FVector SnappedMin = FVector(
        FMath::GridSnap(RawMin.X, CellSize),
        FMath::GridSnap(RawMin.Y, CellSize),
        RawMin.Z
    );
    
    TargetAsset->BakedHeights.Empty();
    TargetAsset->BakedHeights.SetNumZeroed(GridX * GridY);
    
    TargetAsset->GridDimensions = FIntPoint(GridX, GridY);
    TargetAsset->GridWorldOrigin = SnappedMin;
    
    // Trace Settings
    float TopZ = Origin.Z + Extent.Z;
    float BottomZ = Origin.Z - Extent.Z;
    TargetAsset->GroundHeight = TopZ;

    // Ignore certain actors
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); 
    for (AActor* Actor : ActorsToIgnore)
    {
        if (Actor)
        {
            Params.AddIgnoredActor(Actor);
        }
    }
    
    // Use a box extent that covers the cell.
    // Allow for some amount of overlap.
    FVector BoxHalfExtent = FVector((CellSize * 0.45f), (CellSize * 0.45f), 1.0f);
    FQuat Rotation = FQuat::Identity;
    
    FCollisionObjectQueryParams QueryParams;
    for (const auto ObjectType : WALL_COLLISION) QueryParams.AddObjectTypesToQuery(ObjectType);

    for (int32 y = 0; y < GridY; ++y) {
        for (int32 x = 0; x < GridX; ++x) {
            int32 Index = (y * GridX) + x;
            
            // Calculate trace start/end in world space
            // Start from the top of the box, end at the bottom
            FVector Start = SnappedMin + FVector(x * CellSize + (CellSize * 0.5f), y * CellSize + (CellSize * 0.5f), 0.0f);
            Start.Z = TopZ;
            FVector End = Start;
            End.Z = BottomZ;
            
            // SweepSingle checks the entire volume of the box as it moves from Start to End
            FHitResult Hit;
            bool bHit = GetWorld()->SweepSingleByObjectType(Hit, Start, End, Rotation, QueryParams, FCollisionShape::MakeBox(BoxHalfExtent), Params);
            if (bHit) { 
                TargetAsset->BakedHeights[Index] = Hit.ImpactPoint.Z;
            } else {
                TargetAsset->BakedHeights[Index] = BottomZ;
            }
            
            TargetAsset->GroundHeight = FMath::Min(TargetAsset->GroundHeight, TargetAsset->BakedHeights[Index]);
        }
    }

    // Persistence Logic
    TargetAsset->Modify();
    
    UE_LOG(LogTemp, Warning, TEXT("Flow Field Baked Successfully! Please save the %s data asset!"), *TargetAsset->GetName());
}

void AFlowFieldVolume::DrawFlowFieldDebug() const
{
    if (!MyLineBatcher) return;
    MyLineBatcher->Flush();

    if (!bShowDebug || !TargetAsset || TargetAsset->BakedHeights.Num() == 0) return;
    
    FVector ViewLocation;
    bool bFoundLocation = false;
    if (UWorld* World = GetWorld())
    {
        // Try to get the Camera Location in the Editor (Viewport)
#if WITH_EDITOR
        if (GIsEditor && !World->IsGameWorld())
        {
            // This looks at the active editor viewport camera
            FEditorViewportClient* ViewportClient = (FEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
            if (ViewportClient)
            {
                ViewLocation = ViewportClient->GetViewLocation();
                bFoundLocation = true;
            }
        }
#endif

        // If not in editor or viewport check failed, try the Player Controller (Runtime)
        if (!bFoundLocation)
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                FRotator ViewRotation;
                PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
                bFoundLocation = true;
            }
        }
    }
    
    // If we still don't have a location, we can't do a distance check
    if (!bFoundLocation) return;
    const float MaxDebugDistSq = FMath::Square(DebugDistance);
    
    const int32 GridX = TargetAsset->GridDimensions.X;
    const float HalfCell = CellSize * 0.5f;

    for (int32 i = 0; i < TargetAsset->BakedHeights.Num(); ++i)
    {
        int32 x = i % GridX;
        int32 y = i / GridX;

        FVector Center = TargetAsset->GridWorldOrigin + FVector(x * CellSize + HalfCell, y * CellSize + HalfCell, 0.0f);
        Center.Z = TargetAsset->BakedHeights[i];
        
        if ((Center - ViewLocation).SizeSquared2D() > MaxDebugDistSq) continue;
        
        MyLineBatcher->DrawBox(
            Center, 
            FVector(HalfCell - 2.0f, HalfCell - 2.0f, 2.0f), 
            FColor::Green, 
            -1,
            0,
            1.f
        );
        
        // Draw the flow vectors
        if (bShowGroundFlowVectors && TargetAsset->GroundFlowVectors.IsValidIndex(i))
        {
            FVector2D Dir2D = TargetAsset->GroundFlowVectors[i];
            if (!Dir2D.IsNearlyZero())
            {
                FVector Dir(Dir2D.X, Dir2D.Y, 0.0f);
                FVector ArrowEnd = Center + (Dir * HalfCell * 0.8f);
                MyLineBatcher->DrawDirectionalArrow(Center, ArrowEnd, 100.0f, FColor::Red, -1, 0, 5.0f);
            }
        }
    }
}

void AFlowFieldVolume::UpdateAllFlowFields()
{
    if (!TargetAsset || TargetAsset->BakedHeights.Num() == 0) return;

    FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
    
    // Check if we actually need to update
    if (FVector::DistSquared(PlayerLocation, LastPlayerLocation) < FMath::Square(RecomputeThreshold)) return;
    LastPlayerLocation = PlayerLocation;
    
    if (MyLineBatcher) MyLineBatcher->Flush();
    
    GenerateTypeSpecificField(PlayerLocation, TargetAsset->GroundFlowVectors);
    
    DrawFlowFieldDebug();
}

void AFlowFieldVolume::GenerateTypeSpecificField(const FVector& PlayerLocation, TArray<FVector2D>& OutVectors) const
{
    if (!TargetAsset) return;
    
    const int32 GridX = TargetAsset->GridDimensions.X;
    const int32 GridY = TargetAsset->GridDimensions.Y;
    const int32 TotalCells = GridX * GridY;
    
    // Find Goal
    TArray<uint32> IntegrationField;
    IntegrationField.Init(UNREACHABLE, TotalCells);
    TArray<int32> PriorityQueue;
    const auto COMPARE_COSTS = [&](int32 A, int32 B) {
        return  IntegrationField[A] < IntegrationField[B];
    };
    
    FVector RelativePos = PlayerLocation - TargetAsset->GridWorldOrigin;
    int32 TargetX = FMath::FloorToInt(RelativePos.X / CellSize);
    int32 TargetY = FMath::FloorToInt(RelativePos.Y / CellSize);
    if (TargetX >= 0 && TargetX < GridX && TargetY >= 0 && TargetY < GridY)
    {
        int32 TargetIndex = (TargetY * GridX) + TargetX;
        IntegrationField[TargetIndex] = 0;
        PriorityQueue.HeapPush(TargetIndex, COMPARE_COSTS);
    }

    // Dijkstra Pass
    while (PriorityQueue.Num() > 0)
    {
        int32 CurrIdx;
        PriorityQueue.HeapPop(CurrIdx, COMPARE_COSTS);
        
        // Check 4 cardinal neighbors
        const int32 CurX = CurrIdx % GridX;
        const int32 CurY = CurrIdx / GridX;

        for (int32 i = 0; i < 4; ++i)
        {
            int32 NeighborX = CurX + NX[i];
            int32 NeighborY = CurY + NY[i];
            int32 NeighborIdx = (NeighborY * GridX) + NeighborX;

            if (NeighborX < 0 || NeighborX >= GridX || NeighborY < 0 || NeighborY >= GridY) continue;
            
            float HeightDiff = TargetAsset->BakedHeights[CurrIdx] - TargetAsset->BakedHeights[NeighborIdx];
            if (HeightDiff <= 0)
            {
                uint32 NewCost = IntegrationField[CurrIdx] + 1 + FMath::Max(HeightDiff / CellSize, 0);
                if (NewCost < IntegrationField[NeighborIdx])
                {
                    IntegrationField[NeighborIdx] = NewCost;
                    PriorityQueue.HeapPush(NeighborIdx, COMPARE_COSTS);
                }
            }
        }
    }

    // Smoothed Vector Generation (Gradient)
    OutVectors.SetNum(TotalCells);
    for (int32 i = 0; i < TotalCells; ++i)
    {
        int32 x = i % GridX;
        int32 y = i / GridX;

        if (IntegrationField[i] == UNREACHABLE)
        {
            OutVectors[i] = FVector2D::ZeroVector;
            continue;
        }

        FVector2D AccumulatedGradient(0, 0);
        
        for (int32 nx = -1; nx <= 1; ++nx) {
            for (int32 ny = -1; ny <= 1; ++ny) {
                if (nx == 0 && ny == 0) continue;

                int32 NeighborX = x + nx;
                int32 NeighborY = y + ny;
                if (NeighborX < 0 || NeighborX >= GridX || NeighborY < 0 || NeighborY >= GridY) continue;

                int32 nIdx = (NeighborY * GridX) + NeighborX;
                if (IntegrationField[nIdx] == UNREACHABLE) continue;
                
                float HeightDiff = TargetAsset->BakedHeights[nIdx] - TargetAsset->BakedHeights[i]; 
                if (HeightDiff > 0) continue;
                
                // Gradient = CurrentCost - NeighborCost
                // If neighbor is cheaper, result is positive, pulling vector toward it
                float Weight = FMath::Max( IntegrationField[nIdx] -  IntegrationField[i], 0.f);
                AccumulatedGradient += FVector2D(nx, ny) * FMath::Square(Weight);
                
            }
        }
        OutVectors[i] = AccumulatedGradient.GetSafeNormal();
    }
}
