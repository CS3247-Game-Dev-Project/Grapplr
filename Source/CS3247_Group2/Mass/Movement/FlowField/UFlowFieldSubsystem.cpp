#include "UFlowFieldSubsystem.h"
#include "AFlowFieldVolume.h"

FVector2D UFlowFieldSubsystem::GetFlowAtLocation(const FVector& Location) const
{
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
	FVector2D V00 = GetRawVectorFromAsset(X0, Y0);
	FVector2D V10 = GetRawVectorFromAsset(X1, Y0);
	FVector2D V01 = GetRawVectorFromAsset(X0, Y1);
	FVector2D V11 = GetRawVectorFromAsset(X1, Y1);

	// Bilinear Interpolation (Lerp X, then Lerp Y)
	FVector2D LerpBottom = FMath::Lerp(V00, V10, AlphaX);
	FVector2D LerpTop = FMath::Lerp(V01, V11, AlphaX);
    
	return FMath::Lerp(LerpBottom, LerpTop, AlphaY).GetSafeNormal();
}

bool UFlowFieldSubsystem::IsLocationNearEdge(const FVector& Location) const
{
	if (!ActiveVolume || !ActiveVolume->TargetAsset) return true;
	
	// Convert World Position to Grid Coordinates (float)
	FVector RelativePos = Location - ActiveVolume->TargetAsset->GridWorldOrigin;
	float GridX = RelativePos.X / ActiveVolume->CellSize;
	float GridY = RelativePos.Y / ActiveVolume->CellSize;

	// Identify the 4 surrounding cells
	int32 X0 = FMath::FloorToInt(GridX);
	int32 Y0 = FMath::FloorToInt(GridY);
	int32 X1 = X0 + 1;
	int32 Y1 = Y0 + 1;
	
	// Heights from data asset
	auto V00 = GetRawHeightFromAsset(X0, Y0);
	auto V10 = GetRawHeightFromAsset(X1, Y0);
	auto V01 = GetRawHeightFromAsset(X0, Y1);
	auto V11 = GetRawHeightFromAsset(X1, Y1);
	
	return !(FMath::Max(FMath::Abs(V00 - V10), FMath::Abs(V01 - V11), FMath::Abs(V00 - V11)) < 0.1f);
}

FVector2D UFlowFieldSubsystem::GetRawVectorFromAsset(int32 X, int32 Y) const
{
	if (!ActiveVolume || !ActiveVolume->TargetAsset) return FVector2D::ZeroVector;
	
	// Clamp coordinates
	int32 SafeX = FMath::Clamp(X, 0, ActiveVolume->TargetAsset->GridDimensions.X - 1);
	int32 SafeY = FMath::Clamp(Y, 0, ActiveVolume->TargetAsset->GridDimensions.Y - 1);
    
	int32 Index = (SafeY * ActiveVolume->TargetAsset->GridDimensions.Y) + SafeX;
	return ActiveVolume->TargetAsset->GroundFlowVectors[Index]; // TODO: refactor to something more flexible
}

float UFlowFieldSubsystem::GetRawHeightFromAsset(int32 X, int32 Y) const
{
	if (!ActiveVolume || !ActiveVolume->TargetAsset) return 0;
	
	// Clamp coordinates
	int32 SafeX = FMath::Clamp(X, 0, ActiveVolume->TargetAsset->GridDimensions.X - 1);
	int32 SafeY = FMath::Clamp(Y, 0, ActiveVolume->TargetAsset->GridDimensions.Y - 1);
    
	int32 Index = (SafeY * ActiveVolume->TargetAsset->GridDimensions.Y) + SafeX;
	return ActiveVolume->TargetAsset->BakedHeights[Index]; 
}
