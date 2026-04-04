#include "UMassDriftProcessor.h"

#include "FMovementFragments.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassCommonTypes.h"
#include "MassMovementFragments.h"
#include "CS3247_Group2/Mass/Damage/FHealthFragments.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"
#include "MovementStyles/UMassSimpleClimberMovementProcessor.h"
#include "MovementStyles/UMassSimpleFlyerMovementProcessor.h"
#include "MovementStyles/UMassSimpleGroundMovementProcessor.h"
#include "Steering/MassSteeringProcessors.h"

UMassDriftProcessor::UMassDriftProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionPriority = 10;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	ExecutionOrder.ExecuteAfter.Add(UMassSimpleGroundMovementProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteAfter.Add(UMassSimpleFlyerMovementProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteAfter.Add(UMassSimpleClimberMovementProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(UMassSteerToMoveTargetProcessor::StaticClass()->GetFName());

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassDriftProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassDriftFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);
	
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassDriftProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
	float PLAYER_DETECTION_RADIUS = 500.0f;
	float WorldTime = GetWorld()->GetTimeSeconds();
	
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& IterContext)
	{
		// FIXME: desired movements must match move target movement to move forward!!
		auto Movements = IterContext.GetMutableFragmentView<FMassDesiredMovementFragment>();
		auto Drifts = IterContext.GetFragmentView<FMassDriftFragment>();
		auto Transforms = IterContext.GetFragmentView<FTransformFragment>();

		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			auto& Movement = Movements[i];
			auto& Drift = Drifts[i];
			auto CurrentLocation = Transforms[i].GetTransform().GetLocation();
			FVector ToPlayer = PlayerLocation - CurrentLocation;
			
			if (ToPlayer.Size2D() <= PLAYER_DETECTION_RADIUS) continue;
			
			// Calculate Drift: rotate the forward vector slightly based on a sine wave (max 30 deg offset)
			const float DriftSmoothing = FMath::Clamp((ToPlayer.Size2D() / PLAYER_DETECTION_RADIUS) - 1.f, 0.f, 1.0f);
			const float SineValue = FMath::Sin((WorldTime + Drift.PhaseOffset) * Drift.DriftFrequency);
			const float DriftAngle = SineValue * 0.5f * Drift.DriftIntensity * DriftSmoothing;
			
			FVector2D RotatedFlow;
			const float CosA = FMath::Cos(DriftAngle);
			const float SinA = FMath::Sin(DriftAngle);
			RotatedFlow.X = Movement.DesiredVelocity.X * CosA - Movement.DesiredVelocity.Y * SinA;
			RotatedFlow.Y = Movement.DesiredVelocity.X * SinA + Movement.DesiredVelocity.Y * CosA;
			
			if (!RotatedFlow.IsNearlyZero())
			{
				Movement.DesiredVelocity.X = RotatedFlow.X;
				Movement.DesiredVelocity.Y = RotatedFlow.Y;
			}
		}
	});
}
