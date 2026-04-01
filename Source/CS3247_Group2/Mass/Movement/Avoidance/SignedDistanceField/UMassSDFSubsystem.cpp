#include "UMassSDFSubsystem.h"

float UMassSDFSubsystem::GetDistanceAtWorldPosition(const FVector& WorldPos) const
{
	if (!TargetAsset) return 10000.0f;
	return TargetAsset->GetDistanceAtWorldPosition(WorldPos);
}
	
FVector UMassSDFSubsystem::GetGradientAtWorldPosition(const FVector& WorldPos) const
{
	if (!TargetAsset) return FVector::ZeroVector;
	return TargetAsset->GetGradientAtWorldPosition(WorldPos);
}