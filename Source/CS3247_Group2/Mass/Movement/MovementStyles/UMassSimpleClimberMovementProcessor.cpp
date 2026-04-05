#include "UMassSimpleClimberMovementProcessor.h"
#include "MassCommonTypes.h"
#include "MassNavigationFragments.h"
#include "MassEntityManager.h"
#include "MassEntityQuery.h"
#include "MassExecutionContext.h"
#include "DrawDebugHelpers.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "CS3247_Group2/Mass/Constants.h"
#include "CS3247_Group2/Mass/Damage/FHealthFragments.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"
#include "CS3247_Group2/Mass/Movement/UEnemyMovementSubsystem.h"
#include "CS3247_Group2/Mass/Movement/Avoidance/USpatialGridUpdateProcessor.h"
#include "CS3247_Group2/Mass/Movement/Avoidance/SignedDistanceField/UMassSDFSubsystem.h"
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
	const float GlobalMovementSpeedMult = GlobalManager ? GlobalManager->GlobalMovementSpeedMultiplier : 1.0f;
	const FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
	const float DeltaTime = Context.GetDeltaTimeSeconds();
	UMassSDFSubsystem* SDFSubsystem = GetWorld()->GetSubsystem<UMassSDFSubsystem>();
	
	constexpr float CLOSE_TO_PLAYER = 500.f;
	constexpr float SDF_AVOIDANCE_RADIUS = 50.0f;
	constexpr float CLIMB_DISTANCE = 100.0f;
	constexpr float CLIMB_MULTIPLIER = 2.0f;

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
			const FVector HeightOffset = FVector(0.0f, 0.0f, SDF_AVOIDANCE_RADIUS - Heights[i].Height / 2.f + 1.0f);
			float MovementMagnitude = Speeds[i].MaxMovementSpeed * GlobalMovementSpeedMult;
			bool bIsClimbing = false;
			
			// Just moving towards the player looks better for climbing than using a flow field.
			FVector FlowForward = ToPlayer.GetSafeNormal2D();
			
			// Use signed distance field to adjust the forward flow, to avoid wall clipping
			if (SDFSubsystem && SDFSubsystem->HasTargetAsset())
			{
				// Ensure no intersection with the ground planes.
				float SDFDistance = SDFSubsystem->GetDistanceAtWorldPosition(CurrentLocation + HeightOffset);
				if (SDFDistance < CLIMB_DISTANCE)
				{
					// Perform more fine-grained climbing logic with raycasts.
					FVector TargetPos = CurrentLocation + FVector(0, 0, 1.0f - Heights[i].Height / 2.f);
					FVector TraceStart = TargetPos;
					FVector TraceEnd = TargetPos + FlowForward.GetSafeNormal2D() * CLIMB_DISTANCE;
					FCollisionObjectQueryParams QueryParams;
					for (const auto ObjectType : WALL_COLLISION) QueryParams.AddObjectTypesToQuery(ObjectType);
					FHitResult Hit;
					if (GetWorld()->LineTraceSingleByObjectType(Hit, TraceStart, TraceEnd, QueryParams))
					{
						// On hit, move up.
						bIsClimbing = true;
						FlowForward = FVector::UpVector;
						MovementMagnitude *= CLIMB_MULTIPLIER;
					} else
					{
						// Some repulsion force (but with correction) if not facing towards wall.
						FVector SDFGradient = SDFSubsystem->GetGradientAtWorldPosition(CurrentLocation + HeightOffset).GetSafeNormal();
						FlowForward = FMath::Lerp(FlowForward, SDFGradient.GetSafeNormal(), 0.2f);
					}
				}
			}

			// Handle gravity tag updates when climbing state changes.
			const FMassEntityHandle Entity = IterContext.GetEntity(i);
			if (!bHasGravity && !bIsClimbing)
			{
				IterContext.Defer().AddTag<FGravityTag>(Entity);
			} else if (bHasGravity && bIsClimbing)
			{
				IterContext.Defer().RemoveTag<FGravityTag>(Entity);
			}
			
			// Update direction
			Movements[i].DesiredVelocity = FMath::VInterpTo(Movements[i].DesiredVelocity, FlowForward * MovementMagnitude, DeltaTime, Speeds[i].VelocityInterpolationSpeed);
			
			// Point at a spot in the direction the flow field wants us to go
			Target.DistanceToGoal = ToPlayer.Size();
			Target.IntentAtGoal = EMassMovementAction::Move;
			if (FlowForward.IsNearlyZero() || (ToPlayer.Size() <= CLOSE_TO_PLAYER && FMath::Abs(ToPlayer.Z) < 10.0f))
			{
				Target.Forward = ToPlayer.GetSafeNormal2D();
				Target.Center = CurrentLocation + (ToPlayer.GetSafeNormal2D() * MovementMagnitude * DeltaTime);
			} else
			{
				Target.Forward = FlowForward;
				Target.Center = CurrentLocation + (FlowForward * MovementMagnitude * DeltaTime); 
			}
		}
	});
}
