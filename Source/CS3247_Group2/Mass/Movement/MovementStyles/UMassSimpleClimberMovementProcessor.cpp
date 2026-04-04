#include "UMassSimpleClimberMovementProcessor.h"
#include "MassCommonTypes.h"
#include "MassNavigationFragments.h"
#include "MassEntityManager.h"
#include "MassEntityQuery.h"
#include "MassExecutionContext.h"
#include "DrawDebugHelpers.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "CS3247_Group2/Mass/Damage/FHealthFragments.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"
#include "CS3247_Group2/Mass/Movement/UEnemyMovementSubsystem.h"
#include "CS3247_Group2/Mass/Movement/Avoidance/USpatialGridUpdateProcessor.h"
#include "CS3247_Group2/Mass/Movement/Avoidance/SignedDistanceField/UMassSDFSubsystem.h"
#include "CS3247_Group2/Mass/Movement/FlowField/UFlowFieldSubsystem.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"

UMassSimpleClimberMovementProcessor::UMassSimpleClimberMovementProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	ExecutionOrder.ExecuteAfter.Add(USpatialGridUpdateProcessor::StaticClass()->GetFName());

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassSimpleClimberMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMovementSpeedFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FHeightFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FSimpleClimberMovementTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassSimpleClimberMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	const UEnemyMovementSubsystem* GlobalManager = GetWorld()->GetSubsystem<UEnemyMovementSubsystem>();
	float GlobalMovementSpeedMult = GlobalManager ? GlobalManager->GlobalMovementSpeedMultiplier : 1.0f;
	UFlowFieldSubsystem* FlowFieldSubsystem = GetWorld()->GetSubsystem<UFlowFieldSubsystem>();
	FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
	float DeltaTime = Context.GetDeltaTimeSeconds();
	UMassSDFSubsystem* SDFSubsystem = GetWorld()->GetSubsystem<UMassSDFSubsystem>();
	
	constexpr float CLOSE_TO_PLAYER = 500.f;
	constexpr float SDF_AVOIDANCE_RADIUS = 50.0f;
	constexpr float CLIMB_DISTANCE = 100.0f;
	constexpr float CLIMB_MULTIPLIER = 10.0f;

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& IterContext)
	{
		const auto Targets = IterContext.GetMutableFragmentView<FMassMoveTargetFragment>();
		const auto Movements = IterContext.GetMutableFragmentView<FMassDesiredMovementFragment>();
		const auto Transforms = IterContext.GetFragmentView<FTransformFragment>();
		const auto Speeds = IterContext.GetFragmentView<FMovementSpeedFragment>();
		const auto Heights = IterContext.GetFragmentView<FHeightFragment>();
		const bool bHasGravity = IterContext.DoesArchetypeHaveTag<FGravityTag>();

		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			auto& Target = Targets[i];
			FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
			FVector ToPlayer = (PlayerLocation - CurrentLocation);
			const FVector HeightOffset = FVector(0.0f, 0.0f, CLIMB_DISTANCE - Heights[i].Height / 2.f + 1.0f);
			float MovementMagnitude = Speeds[i].MaxMovementSpeed * GlobalMovementSpeedMult;
			bool bIsCurrentlyClimbing = false;
			
			// Get our smoothed flow direction from the subsystem
			// const FVector2D FlowDir2D = FlowFieldSubsystem->GetFlowAtLocation(CurrentLocation, false);
			// FVector FlowForward = FVector(FlowDir2D.X, FlowDir2D.Y, 0.0f).GetSafeNormal();
			// Just moving towards the player looks better
			FVector FlowForward = ToPlayer.GetSafeNormal2D();
			
			// Use signed distance field to adjust the forward flow, to avoid wall clipping
			if (SDFSubsystem && SDFSubsystem->HasTargetAsset())
			{
				// Ensure no intersection with the ground planes.
				float SDFDistance = SDFSubsystem->GetDistanceAtWorldPosition(CurrentLocation + HeightOffset);
				if (SDFDistance < CLIMB_DISTANCE)
				{
					FVector SDFGradient = SDFSubsystem->GetGradientAtWorldPosition(CurrentLocation + HeightOffset).GetSafeNormal();
					float FlowToWall = FVector::DotProduct(FlowForward, SDFGradient);
					// bool bIsVerticalWall = FMath::Abs(SDFGradient.Z) < 0.5f;
					// if (FlowFieldSubsystem->IsLocationNearEdge(CurrentLocation)) {
					FVector ClimbFlow = FVector::ZeroVector;
					if (FlowToWall < 0)
					{
						// Remove gravity tag, climb up, with no repulsion force
						bIsCurrentlyClimbing = true;
						ClimbFlow = (FVector::UpVector + SDFGradient * -0.0001f).GetSafeNormal();
						
						Target.Center = CurrentLocation + (ClimbFlow * MovementMagnitude * DeltaTime); 
						Target.Forward = ClimbFlow;
					}
					
					float WallCloseness = (SDF_AVOIDANCE_RADIUS - SDFDistance) / SDF_AVOIDANCE_RADIUS;
					FVector RepulsionForce = SDFGradient * MovementMagnitude * FMath::Square(WallCloseness);
					FlowForward = FMath::Lerp(ClimbFlow * CLIMB_MULTIPLIER, RepulsionForce.GetSafeNormal(), WallCloseness);
				}
			}

			// Handle gravity tag updates
			const FMassEntityHandle Entity = IterContext.GetEntity(i);
			if (!bHasGravity && !bIsCurrentlyClimbing)
			{
				IterContext.Defer().AddTag<FGravityTag>(Entity);
			} else if (bHasGravity && bIsCurrentlyClimbing)
			{
				IterContext.Defer().RemoveTag<FGravityTag>(Entity);
			}
			
			// Update direction
			Movements[i].DesiredVelocity = FMath::VInterpTo(Movements[i].DesiredVelocity, FlowForward * MovementMagnitude, DeltaTime, Speeds[i].VelocityInterpolationSpeed);
		
			// Target/desire is different if currently climbing, we don't consider sdf repulsion forces.
			if (bIsCurrentlyClimbing) return;
			
			// Point at a spot in the direction the flow field wants us to go
			Target.DistanceToGoal = ToPlayer.Size();
			Target.IntentAtGoal = EMassMovementAction::Move;
			if (FlowForward.IsNearlyZero() || (ToPlayer.Size() <= CLOSE_TO_PLAYER && FMath::Abs(ToPlayer.Z) < 100.0f))
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
