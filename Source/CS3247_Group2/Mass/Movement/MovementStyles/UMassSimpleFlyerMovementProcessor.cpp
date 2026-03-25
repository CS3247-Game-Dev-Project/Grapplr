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
	const float MIN_HEIGHT_ABOVE = 3000.f;
	const float CLOSE_TO_PLAYER = 1000.f;
	const float HEIGHT_DRIFT_MAGNITUDE = 2.0f;
	
	// Use a Sine wave for organic "floaty" movement
	float WorldTime = GetWorld()->GetTimeSeconds();
	float VerticalBob = FMath::Sin(WorldTime * 2.0f) * 500.0f; // Adjust frequency/amplitude

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& IterContext)
	{
		const auto Targets = IterContext.GetMutableFragmentView<FMassMoveTargetFragment>();
		const auto Movements = IterContext.GetMutableFragmentView<FMassDesiredMovementFragment>();
		const auto Transforms = IterContext.GetFragmentView<FTransformFragment>();
		const auto Speeds = IterContext.GetFragmentView<FMovementSpeedFragment>();

		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
			FVector ToPlayer = PlayerLocation - CurrentLocation;
			float CurrentMaxSpeed = Speeds[i].MaxMovementSpeed * GlobalMovementSpeedMult;
			
			// Use Present/Future terrain height
			float MaxTerrainHeight = FMath::Max(
				FlowFieldSubsystem->GetHeightAtLocation(CurrentLocation),
				FlowFieldSubsystem->GetHeightAtLocation(CurrentLocation + 
					FlowFieldSubsystem->GetCellSize() * ToPlayer.GetSafeNormal2D()));
			

			// Update Target Fragment for Mass Steering processors to consume
			auto& Target = Targets[i];
			Target.DistanceToGoal = ToPlayer.Size();
			Target.IntentAtGoal = EMassMovementAction::Move;
			
			if (ToPlayer.Size2D() <= CLOSE_TO_PLAYER)
			{
				// Calculate Steering Logic
				float DesiredZ = MaxTerrainHeight + MIN_HEIGHT_ABOVE;
				FVector DesiredVelocity = ToPlayer.GetSafeNormal() * CurrentMaxSpeed;
				DesiredVelocity.Z = (DesiredZ - CurrentLocation.Z) * HEIGHT_DRIFT_MAGNITUDE;
				
				// Apply Smooth Interpolation for velocity.
				Movements[i].DesiredVelocity = FMath::VInterpTo(Movements[i].DesiredVelocity, DesiredVelocity, DeltaTime, 3.0f);
				Target.Forward = ToPlayer.GetSafeNormal();
				Target.Center = CurrentLocation + ToPlayer.GetSafeNormal() * CurrentMaxSpeed * DeltaTime;
			} else
			{
				// Calculate Steering Logic
				float DesiredZ = MaxTerrainHeight + MIN_HEIGHT_ABOVE + VerticalBob;
				FVector DesiredVelocity = ToPlayer.GetSafeNormal() * CurrentMaxSpeed;
				DesiredVelocity.Z = (DesiredZ - CurrentLocation.Z) * HEIGHT_DRIFT_MAGNITUDE;
				
				// Apply Smooth Interpolation for velocity.
				Movements[i].DesiredVelocity = FMath::VInterpTo(Movements[i].DesiredVelocity, DesiredVelocity, DeltaTime, 3.0f);
				Target.Forward = Movements[i].DesiredVelocity.GetSafeNormal();
				Target.Center = CurrentLocation + Movements[i].DesiredVelocity * DeltaTime;
			}
		}
	});
	
	// EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& IterContext)
	// {
	// 	const auto Targets = IterContext.GetMutableFragmentView<FMassMoveTargetFragment>();
	// 	const auto Transforms = IterContext.GetFragmentView<FTransformFragment>();
	// 	const auto Speeds = IterContext.GetFragmentView<FMovementSpeedFragment>();
	// 	
	// 	for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
	// 	{
	// 		auto& Target = Targets[i];
	// 		FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
	// 		FVector ToPlayer = (PlayerLocation - CurrentLocation);
	// 		float TargetMagnitude = Speeds[i].MaxMovementSpeed * GlobalMovementSpeedMult * DeltaTime;
	// 		
	// 		// Set target to the player.
	// 		auto BaseHeight = FlowFieldSubsystem->GetHeightAtLocation(CurrentLocation);
	// 		// auto HeightAbove = CurrentLocation.Z - BaseHeight;
	// 		if (ToPlayer.Size2D() <= CLOSE_TO_PLAYER)
	// 		{
	// 			Target.DistanceToGoal = ToPlayer.Size();
	// 			Target.IntentAtGoal = EMassMovementAction::Move;
	// 			Target.Forward = ToPlayer.GetSafeNormal2D();
	// 			
	// 			// Simple heuristic for flight.
	// 			Target.Center = CurrentLocation + (ToPlayer.GetSafeNormal() * TargetMagnitude);
	// 			// Target.Center.Z += ToPlayer.Size2D() / 2.0f;
	// 			continue;	
	// 		}
	// 		
	// 		// Target height drift is random.
	// 		// float TargetHeight = FMath::Clamp(HeightAbove + FMath::RandRange(-MAX_DRIFT, MAX_DRIFT), MIN_HEIGHT_ABOVE, MAX_HEIGHT_ABOVE);
	// 		// Otherwise Point at a spot in the direction the flow field wants us to go
	// 		Target.DistanceToGoal = ToPlayer.Size();
	// 		Target.IntentAtGoal = EMassMovementAction::Move;
	// 		Target.Forward = ToPlayer.GetSafeNormal2D();	
	// 		Target.Center = CurrentLocation + (ToPlayer.GetSafeNormal2D() * TargetMagnitude);
	// 		Target.Center.Z = BaseHeight + MIN_HEIGHT_ABOVE; 
	// 	}
	// });
}
