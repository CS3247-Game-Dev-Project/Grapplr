#include "AFlowFieldVolume.h"
#include "DrawDebugHelpers.h"
#include "UFlowFieldSubsystem.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"

AFlowFieldVolume::AFlowFieldVolume()
{
    BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("GridBounds"));
    RootComponent = BoxComponent;

    // Set default size (e.g., 10m x 10m x 5m)
    BoxComponent->SetBoxExtent(FVector(500.0f, 500.0f, 250.0f));
    
    // Ensure it doesn't block the trace we're about to run
    BoxComponent->SetCollisionProfileName(TEXT("NoCollision")); 
    BoxComponent->SetCanEverAffectNavigation(false);
    
    // Create a local line batcher
    MyLineBatcher = CreateDefaultSubobject<ULineBatchComponent>(TEXT("LocalLineBatcher"));
    
    // Ensure it doesn't render in the actual game, only the editor
    MyLineBatcher->SetIsVisualizationComponent(true);
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
    if (!TargetAsset || !BoxComponent) return;

    // Get the actual scaled extent and world center from the BoxComponent
    FVector Extent = BoxComponent->GetScaledBoxExtent();
    FVector Origin = BoxComponent->GetComponentLocation();

    // The rest of the logic remains the same, 
    // but now Extent.X/Y/Z will actually have values!
    int32 GridX = FMath::FloorToInt((Extent.X * 2) / CellSize);
    int32 GridY = FMath::FloorToInt((Extent.Y * 2) / CellSize);
    
    FVector RawMin = Origin - Extent;
    FVector SnappedMin = FVector(
        FMath::GridSnap(RawMin.X, CellSize),
        FMath::GridSnap(RawMin.Y, CellSize),
        RawMin.Z
    );
    
    TargetAsset->BakedCosts.Empty();
    TargetAsset->BakedCosts.SetNumZeroed(GridX * GridY);
    TargetAsset->BakedHeights.Empty();
    TargetAsset->BakedHeights.SetNumZeroed(GridX * GridY);
    
    TargetAsset->GridDimensions = FIntPoint(GridX, GridY);
    TargetAsset->GridWorldOrigin = SnappedMin;
    
    // 3. Trace Settings
    float TopZ = Origin.Z + Extent.Z;
    float BottomZ = Origin.Z - Extent.Z;

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    for (int32 y = 0; y < GridY; ++y)
    {
        for (int32 x = 0; x < GridX; ++x)
        {
            int32 Index = (y * GridX) + x;
            
            // Calculate trace start/end in world space
            // Start from the top of the box, end at the bottom
            FVector Start = SnappedMin + FVector(x * CellSize + (CellSize * 0.5f), y * CellSize + (CellSize * 0.5f), 0.0f);
            Start.Z = TopZ;
            FVector End = Start;
            End.Z = BottomZ;

            FHitResult Hit;
            if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
            { 
                // TODO: use ground height in this calculation
                // If it's too high (e.g., hitting a building roof), increase cost.
                // Or if your buildings are just obstacles, you might use physical materials.
                // For now, let's assume if it hits anything, it's cost 1 (walkable).
                TargetAsset->BakedHeights[Index] = Hit.Location.Z;
                TargetAsset->BakedCosts[Index] = 1;
            } else {
                // Hit nothing? It's a void/cliff. Block it.
                TargetAsset->BakedCosts[Index] = 255;
                TargetAsset->BakedHeights[Index] = BottomZ;
            }
        }
    }

    // 2. Persistence Logic
    TargetAsset->Modify();
    
    UE_LOG(LogTemp, Warning, TEXT("Flow Field Baked Successfully! Please save the %s data asset!"), *TargetAsset->GetName());
}

void AFlowFieldVolume::DrawFlowFieldDebug() const
{
    if (!MyLineBatcher) return;
    MyLineBatcher->Flush();

    if (!bShowDebug || !TargetAsset || TargetAsset->BakedCosts.Num() == 0) return;
    
    const int32 GridX = TargetAsset->GridDimensions.X;
    const float HalfCell = CellSize * 0.5f;

    for (int32 i = 0; i < TargetAsset->BakedCosts.Num(); ++i)
    {
        int32 x = i % GridX;
        int32 y = i / GridX;

        FVector Center = TargetAsset->GridWorldOrigin + FVector(x * CellSize + HalfCell, y * CellSize + HalfCell, 0.0f);
        Center.Z = TargetAsset->BakedHeights[i];
        FColor CellColor = (TargetAsset->BakedCosts[i] >= 255) ? FColor::Red : FColor::Green;

        MyLineBatcher->DrawBox(
            Center, 
            FVector(HalfCell - 2.0f, HalfCell - 2.0f, 2.0f), 
            CellColor, 
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
                MyLineBatcher->DrawLine(Center, ArrowEnd, FColor::Red, 0, 5.0f);
            }
        }
        if (bShowClimbFlowVectors && TargetAsset->ClimbFlowVectors.IsValidIndex(i))
        {
            FVector2D Dir2D = TargetAsset->ClimbFlowVectors[i];
            if (!Dir2D.IsNearlyZero())
            {
                FVector Dir(Dir2D.X, Dir2D.Y, 0.0f);
                FVector ArrowEnd = Center + (Dir * HalfCell * 0.8f);
                MyLineBatcher->DrawLine(Center, ArrowEnd, FColor::Purple, 0, 5.0f);
            }
        }
    }
}

void AFlowFieldVolume::UpdateAllFlowFields()
{
    if (!TargetAsset || TargetAsset->BakedCosts.Num() == 0) return;

    FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
    
    // Check if we actually need to update
    if (FVector::DistSquared(PlayerLocation, LastPlayerLocation) < FMath::Square(RecomputeThreshold)) return;
    LastPlayerLocation = PlayerLocation;
    
    if (MyLineBatcher) MyLineBatcher->Flush();
    
    GenerateTypeSpecificField(PlayerLocation, TargetAsset->GroundFlowVectors, false);
    GenerateTypeSpecificField(PlayerLocation, TargetAsset->ClimbFlowVectors, true);
    
    DrawFlowFieldDebug();
}

void AFlowFieldVolume::GenerateTypeSpecificField(const FVector& PlayerLocation, TArray<FVector2D>& OutVectors, bool bIsClimbing) const
{
    if (!TargetAsset) return;
    
    const int32 GridX = TargetAsset->GridDimensions.X;
    const int32 GridY = TargetAsset->GridDimensions.Y;
    const int32 TotalCells = GridX * GridY;
    
    // TODO: cache and reuse previous values.
    TArray<uint32> IntegrationField;
    IntegrationField.Init(UNREACHABLE, TotalCells);
    TArray<int32> PriorityQueue;
    auto CompareCosts = [&](int32 A, int32 B) {
        return IntegrationField[A] < IntegrationField[B];
    };

    // Find Goal
    FVector RelativePos = PlayerLocation - TargetAsset->GridWorldOrigin;
    int32 TargetX = FMath::FloorToInt(RelativePos.X / CellSize);
    int32 TargetY = FMath::FloorToInt(RelativePos.Y / CellSize);
    if (TargetX >= 0 && TargetX < GridX && TargetY >= 0 && TargetY < GridY)
    {
        int32 TargetIndex = (TargetY * GridX) + TargetX;
        IntegrationField[TargetIndex] = 0;
        PriorityQueue.HeapPush(TargetIndex, CompareCosts);
    }

    // Dijkstra Pass
    while (PriorityQueue.Num() > 0)
    {
        int32 CurrIdx;
        PriorityQueue.HeapPop(CurrIdx, CompareCosts);
        
        // Check 4 cardinal neighbors
        const int32 CurX = CurrIdx % GridX;
        const int32 CurY = CurrIdx / GridX;
        const int32 NX[4] = {1, -1, 0, 0};
        const int32 NY[4] = {0, 0, 1, -1};

        for (int32 i = 0; i < 4; ++i)
        {
            int32 NeighborX = CurX + NX[i];
            int32 NeighborY = CurY + NY[i];

            if (NeighborX < 0 || NeighborX >= GridX || NeighborY < 0 || NeighborY >= GridY) continue;

            int32 NeighborIdx = (NeighborY * GridX) + NeighborX;
            
            // Validation
            bool bValid = (TargetAsset->BakedCosts[NeighborIdx] < 255);
            float HeightDiff = TargetAsset->BakedHeights[CurrIdx] - TargetAsset->BakedHeights[NeighborIdx];
            if ((bIsClimbing || HeightDiff <= 0) && bValid)
            {
                uint32 NewCost = IntegrationField[CurrIdx] + TargetAsset->BakedCosts[NeighborIdx] + FMath::Max(HeightDiff / CellSize, 0);
                if (NewCost < IntegrationField[NeighborIdx])
                {
                    IntegrationField[NeighborIdx] = NewCost;
                    PriorityQueue.HeapPush(NeighborIdx, CompareCosts);
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
                float Weight = FMath::Max(IntegrationField[nIdx] - IntegrationField[i], 0.f);
                AccumulatedGradient += FVector2D(nx, ny) * FMath::Square(Weight);
                
            }
        }
        OutVectors[i] = AccumulatedGradient.GetSafeNormal();
    }
}
