#include "UMassSimpleGroundMovementProcessor.h"

#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassSignalSubsystem.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"
#include "CS3247_Group2/Mass/Movement/UEnemyMovementSubsystem.h"
#include "CS3247_Group2/Mass/Movement/Avoidance/USpatialGridUpdateProcessor.h"
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
	
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassSimpleGroundMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	const UEnemyMovementSubsystem* GlobalManager = GetWorld()->GetSubsystem<UEnemyMovementSubsystem>();
	float GlobalMovementSpeedMult = GlobalManager ? GlobalManager->GlobalMovementSpeedMultiplier : 1.0f;
	UFlowFieldSubsystem* FlowFieldSubsystem = GetWorld()->GetSubsystem<UFlowFieldSubsystem>();
	constexpr float CLOSE_TO_PLAYER = 500.f;
	FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
	float DeltaTime = Context.GetDeltaTimeSeconds();

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
			float TargetMagnitude = Speeds[i].MaxMovementSpeed * GlobalMovementSpeedMult * DeltaTime;
			
			// Get our smoothed flow direction from the subsystem
			const FVector2D FlowDir2D = FlowFieldSubsystem->GetFlowAtLocation(CurrentLocation);
			FVector FlowForward = FVector(FlowDir2D.X, FlowDir2D.Y, 0.0f).GetSafeNormal();
			Movements[i].DesiredVelocity = FMath::VInterpTo(Movements[i].DesiredVelocity, FlowForward * TargetMagnitude, DeltaTime, 3.0f);
			
			// Point at a spot in the direction the flow field wants us to go
			Target.DistanceToGoal = ToPlayer.Size();
			Target.IntentAtGoal = EMassMovementAction::Move;
			if (FlowForward.IsNearlyZero() || ToPlayer.Size() <= CLOSE_TO_PLAYER)
			{
				Target.Forward = ToPlayer.GetSafeNormal2D();	
				Target.Center = CurrentLocation + (ToPlayer.GetSafeNormal2D() * TargetMagnitude);
			} else
			{
				Target.Center = CurrentLocation + (FlowForward * TargetMagnitude); 
				Target.Forward = FlowForward;
			}
		}
	});
}
