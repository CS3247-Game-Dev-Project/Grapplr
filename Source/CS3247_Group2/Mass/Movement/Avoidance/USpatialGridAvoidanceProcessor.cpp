#include "USpatialGridAvoidanceProcessor.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "UMassSpatialGridSubsystem.h"
#include "USpatialGridUpdateProcessor.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"
#include "Steering/MassSteeringProcessors.h"

USpatialGridAvoidanceProcessor::USpatialGridAvoidanceProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionPriority = 0;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::ApplyForces;
	
	// FIXME: if avoidance gets reordered after steer to, then the update will be lost.
	ExecutionOrder.ExecuteAfter.Add(USpatialGridUpdateProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(UMassSteerToMoveTargetProcessor::StaticClass()->GetFName());

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void USpatialGridAvoidanceProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FSpatialGridAvoidanceFragment>(EMassFragmentAccess::ReadOnly);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void USpatialGridAvoidanceProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
    auto SpatialGridSubsystem = GetWorld()->GetSubsystem<UMassSpatialGridSubsystem>();
    const float DeltaTime = Context.GetDeltaTimeSeconds();

    const float AVOIDANCE_STRENGTH = 100.0f;
    const float AVOIDANCE_WEIGHT = 0.9f;
    const float PERSONAL_SPACE = 0.f;

    EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& IterContext)
    {
    	const auto Targets = IterContext.GetMutableFragmentView<FMassMoveTargetFragment>();
		const auto Transforms = IterContext.GetMutableFragmentView<FTransformFragment>();
		const auto Movements = IterContext.GetMutableFragmentView<FMassDesiredMovementFragment>();
		const auto Avoidances = IterContext.GetFragmentView<FSpatialGridAvoidanceFragment>();
        
        for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
        {
        	int32 CurrentIdx = IterContext.GetEntity(i).Index;
            FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
        	auto& Target = Targets[i];
            float Radius = Avoidances[i].AvoidanceSpaceRadius;
            
            FVector TotalSeparation = FVector::ZeroVector;
        	
            TArray<int32> NeighborIndices;
            SpatialGridSubsystem->GetEntitiesInRadius(CurrentLocation, Radius + PERSONAL_SPACE, NeighborIndices);
            for (int32 OtherIdx : NeighborIndices)
            {
                if (OtherIdx == CurrentIdx) continue;
            	if (OtherIdx >= SpatialGridSubsystem->MAX_ENTITIES)
            	{
					UE_LOG(LogTemp, Warning, TEXT("%s: Entity Index %d is out of bounds"), *GetName(), OtherIdx);
            		continue;
            	}

                FVector OtherPos = SpatialGridSubsystem->GetLocationFromIdx(OtherIdx);
                FVector ToOther = CurrentLocation - OtherPos;
                float DistSq = ToOther.SizeSquared();
                float CombinedRadius = Radius + PERSONAL_SPACE;
                
                if (DistSq < FMath::Square(CombinedRadius) && DistSq > 0.001f)
                {
                    // Smoothing: non-linear push.
                    // Closer = Much stronger force. Edge = Almost zero force.
                    float Strength = FMath::Clamp(1.0f - (ToOther.Size() / CombinedRadius), 0.0f, 1.0f) * AVOIDANCE_STRENGTH;
                    
                	TotalSeparation += ToOther * Strength;
                }
            }

        	// Apply separation force to the target movement
            // Target.Forward = FMath::Lerp(Target.Forward, TotalSeparation * DeltaTime, AVOIDANCE_WEIGHT);
        	Movements[i].DesiredVelocity = FMath::Lerp(Movements[i].DesiredVelocity, TotalSeparation * DeltaTime, AVOIDANCE_WEIGHT);
        	
        	// Velocities[i].Value = TotalSeparation * DeltaTime;
        	
        	// Secondary update to location, in case target movement force applications are reordered in the processor pipeline,
        	// resulting in the separation force not being applied.
        	// Transforms[i].GetMutableTransform().SetTranslation(CurrentLocation + Target.Forward * 0.001f);
        }
    });
}
