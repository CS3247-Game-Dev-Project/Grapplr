#include "UMassSimpleGroundMovementProcessor.h"
#include "MassExecutionContext.h"
#include "MassNavigationFragments.h"
#include "MassSignalSubsystem.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"
#include "CS3247_Group2/Mass/Movement/FlowField/UFlowFieldSubsystem.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"

UMassSimpleGroundMovementProcessor::UMassSimpleGroundMovementProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	ExecutionOrder.ExecuteBefore.Add(TEXT("SpatialGridAvoidanceProcessor"));

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassSimpleGroundMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassDriftFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FHeightFragment>(EMassFragmentAccess::ReadOnly);
	
	EntityQuery.AddTagRequirement<FSimpleGroundMovementTag>(EMassFragmentPresence::All);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassSimpleGroundMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	UFlowFieldSubsystem* FlowFieldSubsystem = GetWorld()->GetSubsystem<UFlowFieldSubsystem>();
	constexpr float STRAIGHT_THRESHOLD = 500.f;
	FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
	
	// Iterate through all entities
	EntityQuery.ForEachEntityChunk(Context, [this, PlayerLocation, FlowFieldSubsystem, STRAIGHT_THRESHOLD](FMassExecutionContext& IterContext)
	{
		const auto MoveTargets = IterContext.GetMutableFragmentView<FMassMoveTargetFragment>();
		const auto Transforms = IterContext.GetFragmentView<FTransformFragment>();
		const auto Drifts = IterContext.GetFragmentView<FMassDriftFragment>();
		const auto Heights = IterContext.GetFragmentView<FHeightFragment>();
		const bool bHasGravity = IterContext.DoesArchetypeHaveTag<FGravityTag>();

		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			auto& MoveTarget = MoveTargets[i];
			FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
			FVector ToPlayer = (PlayerLocation - CurrentLocation);
			
			// Get our smoothed flow direction from the subsystem
			const FVector2D FlowDir2D = FlowFieldSubsystem->GetFlowAtLocation(CurrentLocation);
			
			// Point at a spot in the direction the flow field wants us to go
			MoveTarget.DistanceToGoal = ToPlayer.Size();
			MoveTarget.IntentAtGoal = EMassMovementAction::Move;

			// Calculate Drift: rotate the flow vector slightly based on a sine wave (base 30 deg offset)
			const FMassDriftFragment& Drift = Drifts[i];
			const float SineValue = FMath::Sin((GetWorld()->TimeSeconds + Drift.PhaseOffset) * Drift.DriftFrequency);
			const float DriftAngle = SineValue * 0.5f * Drift.DriftIntensity;
			FVector2D RotatedFlow;
			const float CosA = FMath::Cos(DriftAngle);
			const float SinA = FMath::Sin(DriftAngle);
			RotatedFlow.X = FlowDir2D.X * CosA - FlowDir2D.Y * SinA;
			RotatedFlow.Y = FlowDir2D.X * SinA + FlowDir2D.Y * CosA;
			
			FVector FlowForward = FVector(RotatedFlow.X, RotatedFlow.Y, 0.0f).GetSafeNormal();
			
			// If very close to player, ignore flow and move straight to target
			if (ToPlayer.Size() <= STRAIGHT_THRESHOLD || FlowForward.IsNearlyZero()) 
			{
				MoveTarget.Forward = ToPlayer.GetSafeNormal();
				MoveTarget.Center = FVector(PlayerLocation.X, PlayerLocation.Y, MoveTarget.Center.Z);
			} else
			{
				MoveTarget.Forward = FlowForward;
				MoveTarget.Center = CurrentLocation + (MoveTarget.Forward * MoveTarget.DistanceToGoal); 
			}
			
			// Simulate always gravity if above the default ground height.
			if (!bHasGravity && FlowFieldSubsystem->GetGroundHeight() + Heights[i].Height / 2.f < CurrentLocation.Z)
			{
				IterContext.Defer().AddTag<FGravityTag>(IterContext.GetEntity(i));
			}
		}
	});
}
