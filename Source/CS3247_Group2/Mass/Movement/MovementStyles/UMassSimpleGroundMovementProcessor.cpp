#include "UMassSimpleGroundMovementProcessor.h"

#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassSignalSubsystem.h"
#include "CS3247_Group2/Mass/Damage/FHealthFragments.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"
#include "CS3247_Group2/Mass/Movement/UEnemyMovementSubsystem.h"
#include "CS3247_Group2/Mass/Movement/Avoidance/USpatialGridUpdateProcessor.h"
#include "CS3247_Group2/Mass/Movement/Avoidance/SignedDistanceField/UMassSDFSubsystem.h"
#include "CS3247_Group2/Mass/Movement/FlowField/UFlowFieldSubsystem.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"

UMassSimpleGroundMovementProcessor::UMassSimpleGroundMovementProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	ExecutionOrder.ExecuteAfter.Add(USpatialGridUpdateProcessor::StaticClass()->GetFName());

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassSimpleGroundMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMovementSpeedFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FSimpleGroundMovementTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);
	
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassSimpleGroundMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	const UEnemyMovementSubsystem* GlobalManager = GetWorld()->GetSubsystem<UEnemyMovementSubsystem>();
	const float GlobalMovementSpeedMult = GlobalManager ? GlobalManager->GlobalMovementSpeedMultiplier : 1.0f;
	const UFlowFieldSubsystem* FlowFieldSubsystem = GetWorld()->GetSubsystem<UFlowFieldSubsystem>();
	const FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
	const float DeltaTime = Context.GetDeltaTimeSeconds();
	const UMassSDFSubsystem* SDFSubsystem = GetWorld()->GetSubsystem<UMassSDFSubsystem>();
	
	constexpr float CLOSE_TO_PLAYER = 500.f;
	constexpr float SDF_AVOIDANCE_RADIUS = 150.0f;
	constexpr float OUT_OF_REACH_SPEED = 50.0f;

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& IterContext)
	{
		const auto Targets = IterContext.GetMutableFragmentView<FMassMoveTargetFragment>();
		const auto Movements = IterContext.GetMutableFragmentView<FMassDesiredMovementFragment>();
		const auto Transforms = IterContext.GetFragmentView<FTransformFragment>();
		const auto Speeds = IterContext.GetFragmentView<FMovementSpeedFragment>();

		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			auto& Target = Targets[i];
			FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
			FVector ToPlayer = (PlayerLocation - CurrentLocation);
			float MovementMagnitude = Speeds[i].MaxMovementSpeed * GlobalMovementSpeedMult;
			
			// Get our smoothed flow direction from the subsystem
			const FVector2D FlowDir2D = FlowFieldSubsystem->GetFlowAtLocation(CurrentLocation);
			FVector FlowForward = FVector(FlowDir2D.X, FlowDir2D.Y, 0.0f).GetSafeNormal();
			if (FlowDir2D.IsNearlyZero())
			{
				// Slow down movement if out of reach.
				MovementMagnitude = FMath::Clamp(MovementMagnitude, 0.f, OUT_OF_REACH_SPEED);	
			}
			
			// Use signed distance field to adjust the forward flow, to avoid wall clipping
			if (SDFSubsystem && SDFSubsystem->HasTargetAsset())
			{
				float SDFDistance = SDFSubsystem->GetDistanceAtWorldPosition(CurrentLocation);
				if (SDFDistance < SDF_AVOIDANCE_RADIUS)
				{
					FVector SDFGradient = SDFSubsystem->GetGradientAtWorldPosition(CurrentLocation).GetSafeNormal2D();
					float FlowToWall = FVector::DotProduct(FlowForward, SDFGradient);
					FVector SlidingFlow = FVector::ZeroVector;
					if (FlowToWall < 0)
					{
						SlidingFlow = FlowForward - (SDFGradient * FlowToWall);
					}
					
					// Ensure flow is flattened to just the ground plane for ground units
					float WallCloseness = (SDF_AVOIDANCE_RADIUS - SDFDistance) / SDF_AVOIDANCE_RADIUS;
					FlowForward = FMath::Lerp(SlidingFlow.GetSafeNormal2D(), SDFGradient.GetSafeNormal2D(), WallCloseness).GetSafeNormal2D();
				}
			}
			
			Movements[i].DesiredVelocity = FMath::VInterpTo(Movements[i].DesiredVelocity, FlowForward * MovementMagnitude, DeltaTime, Speeds[i].VelocityInterpolationSpeed);
			
			// Point at a spot in the direction the flow field wants us to go
			Target.DistanceToGoal = ToPlayer.Size();
			Target.IntentAtGoal = EMassMovementAction::Move;
			if (FlowForward.IsNearlyZero() || ToPlayer.Size() <= CLOSE_TO_PLAYER)
			{
				Target.Forward = ToPlayer.GetSafeNormal2D();
				Target.Center = CurrentLocation + (ToPlayer.GetSafeNormal2D() * MovementMagnitude * DeltaTime);
			} else
			{
				Target.Center = CurrentLocation + (FlowForward * MovementMagnitude * DeltaTime); 
				Target.Forward = FlowForward;
			}
		}
	});
}
