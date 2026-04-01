#include "UMassSimpleFlyerMovementProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassNavigationFragments.h"
#include "MassEntityManager.h"
#include "MassEntityQuery.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"
#include "CS3247_Group2/Mass/Movement/UEnemyMovementSubsystem.h"
#include "CS3247_Group2/Mass/Movement/Avoidance/USpatialGridUpdateProcessor.h"
#include "CS3247_Group2/Mass/Movement/Avoidance/SignedDistanceField/UMassSDFSubsystem.h"
#include "CS3247_Group2/Mass/Movement/FlowField/UFlowFieldSubsystem.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"

UMassSimpleFlyerMovementProcessor::UMassSimpleFlyerMovementProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	ExecutionOrder.ExecuteAfter.Add(USpatialGridUpdateProcessor::StaticClass()->GetFName());

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassSimpleFlyerMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMovementSpeedFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSimpleFlyerConfigFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FSimpleFlyerMovementTag>(EMassFragmentPresence::All);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassSimpleFlyerMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	const UEnemyMovementSubsystem* GlobalManager = GetWorld()->GetSubsystem<UEnemyMovementSubsystem>();
	float GlobalMovementSpeedMult = GlobalManager ? GlobalManager->GlobalMovementSpeedMultiplier : 1.0f;
	float DeltaTime = Context.GetDeltaTimeSeconds();
	FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
	UFlowFieldSubsystem* FlowFieldSubsystem = GetWorld()->GetSubsystem<UFlowFieldSubsystem>();
	UMassSDFSubsystem* SDFSubsystem = GetWorld()->GetSubsystem<UMassSDFSubsystem>();
	
	const float MIN_HEIGHT_ABOVE = 3000.f;
	const float CLOSE_TO_PLAYER = 2000.f;
	const float VERY_CLOSE_TO_PLAYER = 500.f;
	const float HEIGHT_DRIFT_MAGNITUDE = 2.0f;
	constexpr float SDF_AVOIDANCE_RADIUS = 150.0f;
	
	float WorldTime = GetWorld()->GetTimeSeconds();

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& IterContext)
	{
		const auto Targets = IterContext.GetMutableFragmentView<FMassMoveTargetFragment>();
		const auto Movements = IterContext.GetMutableFragmentView<FMassDesiredMovementFragment>();
		const auto Transforms = IterContext.GetFragmentView<FTransformFragment>();
		const auto Speeds = IterContext.GetFragmentView<FMovementSpeedFragment>();
		const auto Configs = IterContext.GetFragmentView<FSimpleFlyerConfigFragment>();

		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
			FVector ToPlayer = PlayerLocation - CurrentLocation;
			float MovementMagnitude = Speeds[i].MaxMovementSpeed * GlobalMovementSpeedMult;
			
			// Use Present/Future terrain height
			float MaxTerrainHeight = FMath::Max(
				FlowFieldSubsystem->GetHeightAtLocation(CurrentLocation),
				FlowFieldSubsystem->GetHeightAtLocation(CurrentLocation + 
					FlowFieldSubsystem->GetCellSize() * ToPlayer.GetSafeNormal2D()));

			// Update Target Fragment for Mass Steering processors to consume
			auto& Target = Targets[i];
			Target.DistanceToGoal = ToPlayer.Size();
			Target.IntentAtGoal = EMassMovementAction::Move;
			
			// Organic floating with a sine wave.
			float VerticalBob = FMath::Sin(WorldTime * Configs[i].FlightHeightFrequency) * Configs[i].FlightHeightAmplitude;
			float DesiredZ = MaxTerrainHeight + MIN_HEIGHT_ABOVE + VerticalBob;
			if (ToPlayer.Size2D() <= CLOSE_TO_PLAYER)
			{
				float AdditionalHeight = FMath::Max(0.f , FMath::Lerp(0.f, MIN_HEIGHT_ABOVE, (ToPlayer.Size2D() - VERY_CLOSE_TO_PLAYER) / CLOSE_TO_PLAYER));
				DesiredZ = FMath::Max(PlayerLocation.Z, MaxTerrainHeight) + AdditionalHeight;
			}
			
			FVector ForwardVector = ToPlayer.GetSafeNormal();
			
			// Use signed distance field to adjust the forward flow, to avoid wall clipping
			if (SDFSubsystem && SDFSubsystem->HasTargetAsset())
			{
				float SDFDistance = SDFSubsystem->GetDistanceAtWorldPosition(CurrentLocation);
				if (SDFDistance < SDF_AVOIDANCE_RADIUS)
				{
					FVector SDFGradient = SDFSubsystem->GetGradientAtWorldPosition(CurrentLocation).GetSafeNormal();
					float FlowToWall = FVector::DotProduct(ForwardVector, SDFGradient);
					FVector SlidingFlow = FVector::ZeroVector;
					if (FlowToWall < 0)
					{
						SlidingFlow = ForwardVector - (SDFGradient * FlowToWall);
					}
					float WallClosenest = (SDF_AVOIDANCE_RADIUS - SDFDistance) / SDF_AVOIDANCE_RADIUS;
					FVector RepulsionForce = SDFGradient * MovementMagnitude * FMath::Square(WallClosenest);
					
					ForwardVector = FMath::Lerp(SlidingFlow.GetSafeNormal(), RepulsionForce.GetSafeNormal(), WallClosenest).GetSafeNormal();
				}
			}
			
			// Calculate Steering Logic
			FVector DesiredVelocity = ForwardVector * MovementMagnitude;
			DesiredVelocity.Z = (DesiredZ - CurrentLocation.Z) * HEIGHT_DRIFT_MAGNITUDE;
			
			// Apply Smooth Interpolation for velocity.
			Movements[i].DesiredVelocity = FMath::VInterpTo(Movements[i].DesiredVelocity, DesiredVelocity, DeltaTime, 3.0f);
			Target.Forward = Movements[i].DesiredVelocity.GetSafeNormal();
			Target.Center = CurrentLocation + Movements[i].DesiredVelocity * DeltaTime;
		}
	});
}
