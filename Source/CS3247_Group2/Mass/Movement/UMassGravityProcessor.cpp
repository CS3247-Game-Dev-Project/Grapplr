#include "UMassGravityProcessor.h"
#include "FMovementFragments.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassCommonTypes.h"
#include "CS3247_Group2/Mass/Constants.h"
#include "CS3247_Group2/Mass/EnemyDeath/FEnemyDrops.h"
#include "FlowField/UFlowFieldSubsystem.h"

UMassGravityProcessor::UMassGravityProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::ApplyForces;
	
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassGravityProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FGravityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite); 
	EntityQuery.AddRequirement<FHeightFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FExpDropFragment>(EMassFragmentAccess::None, EMassFragmentPresence::None); // Only apply to enemies
	EntityQuery.AddTagRequirement<FGravityTag>(EMassFragmentPresence::All);
	
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassGravityProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	const float DeltaTime = Context.GetDeltaTimeSeconds();
	UFlowFieldSubsystem* FlowFieldSubsystem = GetWorld()->GetSubsystem<UFlowFieldSubsystem>();
	
	EntityQuery.ForEachEntityChunk(Context, [this, DeltaTime, FlowFieldSubsystem](FMassExecutionContext& IterContext)
	{
		auto Transforms = IterContext.GetMutableFragmentView<FTransformFragment>();
		auto Gravities = IterContext.GetMutableFragmentView<FGravityFragment>();
		auto Heights = IterContext.GetFragmentView<FHeightFragment>();
		
		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			const FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
			const float HalfHeight = Heights[i].Height / 2.f;
	
			// Skip the gravity computation if already on the ground
			if ((FlowFieldSubsystem->GetGroundHeight() + HalfHeight + 0.1f >= CurrentLocation.Z) && (Gravities[i].AccumulatedVelocity == 0))
			{
				float GroundZ = FlowFieldSubsystem->GetGroundHeight() + HalfHeight;
				Transforms[i].GetMutableTransform().SetTranslation(FVector(CurrentLocation.X, CurrentLocation.Y, GroundZ));
				continue;
			}
			
			// Perform a trace, preventing clipping into the ground.
			FVector TargetPos = CurrentLocation + FVector(0, 0, -HalfHeight);
			FVector TraceStart = TargetPos + FVector(0, 0, Heights[i].Height); 
			FVector TraceEnd = TargetPos + FVector(0, 0, Gravities[i].AccumulatedVelocity * DeltaTime);
			FCollisionQueryParams Params;
			FHitResult Hit;
			if (GetWorld()->LineTraceSingleByObjectType(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
			{
				// Snap to the ground if hit.
				float GroundZ = Hit.ImpactPoint.Z + HalfHeight;
				Transforms[i].GetMutableTransform().SetTranslation(FVector(CurrentLocation.X, CurrentLocation.Y, GroundZ));
				Gravities[i].AccumulatedVelocity = 0;
			} else {
				Transforms[i].GetMutableTransform().SetTranslation(CurrentLocation + FVector(0, 0, Gravities[i].AccumulatedVelocity * DeltaTime));
				
				// Keep adding gravity force if no ground hit.
				Gravities[i].AccumulatedVelocity += GRAVITY * DeltaTime;
			}
		}
	});
}
