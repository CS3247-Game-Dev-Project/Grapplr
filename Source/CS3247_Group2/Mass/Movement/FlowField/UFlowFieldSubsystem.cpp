#include "UFlowFieldSubsystem.h"
#include "AFlowFieldVolume.h"

FVector2D UFlowFieldSubsystem::GetFlowAtLocation(const FVector& Location, const bool IsGroundFlow) const
{
	WarnIfNoAsset();
	if (!ActiveVolume || !ActiveVolume->TargetAsset) return FVector2D::ZeroVector;
	
	// Convert World Position to Grid Coordinates (float)
	FVector RelativePos = Location - ActiveVolume->TargetAsset->GridWorldOrigin;
	float GridX = RelativePos.X / ActiveVolume->CellSize;
	float GridY = RelativePos.Y / ActiveVolume->CellSize;

	// Identify the 4 surrounding cells
	int32 X0 = FMath::FloorToInt(GridX);
	int32 Y0 = FMath::FloorToInt(GridY);
	int32 X1 = X0 + 1;
	int32 Y1 = Y0 + 1;

	// Calculate Alpha (fractional distance between cells)
	float AlphaX = GridX - X0;
	float AlphaY = GridY - Y0;

	// Vectors from data asset
	FVector2D V00 = GetRawVectorFromAsset(X0, Y0, IsGroundFlow);
	FVector2D V10 = GetRawVectorFromAsset(X1, Y0, IsGroundFlow);
	FVector2D V01 = GetRawVectorFromAsset(X0, Y1, IsGroundFlow);
	FVector2D V11 = GetRawVectorFromAsset(X1, Y1, IsGroundFlow);

	// Bilinear Interpolation (Lerp X, then Lerp Y)
	FVector2D LerpBottom = FMath::Lerp(V00, V10, AlphaX);
	FVector2D LerpTop = FMath::Lerp(V01, V11, AlphaX);
    
	return FMath::Lerp(LerpBottom, LerpTop, AlphaY).GetSafeNormal();
}

bool UFlowFieldSubsystem::IsLocationNearEdge(const FVector& Location) const
{
	WarnIfNoAsset();
	if (!ActiveVolume || !ActiveVolume->TargetAsset) return true;
	
	auto Coords = ConvertToGridCoords(Location);
	
	// Identify the surrounding cells
	float ExpectedHeight = GetRawHeightFromAsset(Coords.X, Coords.Y);
	for (int32 nx = -1; nx <= 1; ++nx) {
		for (int32 ny = -1; ny <= 1; ++ny) {
			if (nx == 0 && ny == 0) continue;

			int32 NeighborX = Coords.X + nx;
			int32 NeighborY = Coords.Y + ny;
			if (NeighborX < 0 || NeighborX >= ActiveVolume->TargetAsset->GridDimensions.X ||
				NeighborY < 0 || NeighborY >= ActiveVolume->TargetAsset->GridDimensions.Y) continue;
			
			if (FMath::Abs(ExpectedHeight - GetRawHeightFromAsset(NeighborX, NeighborY)) >= 0.1f)
			{
				return true;
			}
		}
	}
	
	return false;
}

FVector2D UFlowFieldSubsystem::ConvertToGridCoords(const FVector& Location) const
{
	WarnIfNoAsset();
	if (!ActiveVolume || !ActiveVolume->TargetAsset) return FVector2D::ZeroVector;
	
	FVector RelativePos = Location - ActiveVolume->TargetAsset->GridWorldOrigin;
	float GridX = RelativePos.X / ActiveVolume->CellSize;
	float GridY = RelativePos.Y / ActiveVolume->CellSize;
	int32 x = FMath::FloorToInt(GridX);
	int32 y = FMath::FloorToInt(GridY);
	
	return FVector2D(x, y);
}

FVector2D UFlowFieldSubsystem::GetRawVectorFromAsset(int32 X, int32 Y, const bool IsGroundFlow) const
{
	WarnIfNoAsset();
	if (!ActiveVolume || !ActiveVolume->TargetAsset) return FVector2D::ZeroVector;
	
	// Clamp coordinates
	int32 SafeX = FMath::Clamp(X, 0, ActiveVolume->TargetAsset->GridDimensions.X - 1);
	int32 SafeY = FMath::Clamp(Y, 0, ActiveVolume->TargetAsset->GridDimensions.Y - 1);
    
	int32 Index = (SafeY * ActiveVolume->TargetAsset->GridDimensions.Y) + SafeX;
	
	if (IsGroundFlow)
	{
		if (ActiveVolume->TargetAsset->GroundFlowVectors.GetAllocatedSize() < Index) return FVector2D::ZeroVector;
		return ActiveVolume->TargetAsset->GroundFlowVectors[Index];
	} else
	{
		if (ActiveVolume->TargetAsset->ClimbFlowVectors.GetAllocatedSize() < Index) return FVector2D::ZeroVector;
		return ActiveVolume->TargetAsset->ClimbFlowVectors[Index];
	}
}

float UFlowFieldSubsystem::GetRawHeightFromAsset(int32 X, int32 Y) const
{
	WarnIfNoAsset();
	if (!ActiveVolume || !ActiveVolume->TargetAsset) return 0;
	
	// Clamp coordinates
	int32 SafeX = FMath::Clamp(X, 0, ActiveVolume->TargetAsset->GridDimensions.X - 1);
	int32 SafeY = FMath::Clamp(Y, 0, ActiveVolume->TargetAsset->GridDimensions.Y - 1);
    
	int32 Index = (SafeY * ActiveVolume->TargetAsset->GridDimensions.Y) + SafeX;
	if (ActiveVolume->TargetAsset->BakedHeights.GetAllocatedSize() < Index) return 0;
	return ActiveVolume->TargetAsset->BakedHeights[Index]; 
}
