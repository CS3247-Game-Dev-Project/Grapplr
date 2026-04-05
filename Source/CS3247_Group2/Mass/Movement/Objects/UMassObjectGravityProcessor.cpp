#include "UMassObjectGravityProcessor.h"

#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassCommonTypes.h"
#include "MassMovementFragments.h"
#include "UMassExpDropMovementProcessor.h"
#include "CS3247_Group2/Mass/Constants.h"
#include "CS3247_Group2/Mass/EnemyDeath/FEnemyDrops.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"
#include "CS3247_Group2/Mass/Movement/FlowField/UFlowFieldSubsystem.h"

UMassObjectGravityProcessor::UMassObjectGravityProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	ExecutionOrder.ExecuteAfter.Add(UMassExpDropMovementProcessor::StaticClass()->GetFName());
	
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassObjectGravityProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FGravityFragment>(EMassFragmentAccess::ReadWrite);
	
	EntityQuery.AddRequirement<FHeightFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FExpDropFragment>(EMassFragmentAccess::None);
	EntityQuery.AddTagRequirement<FGravityTag>(EMassFragmentPresence::All);
	
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassObjectGravityProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	const float DeltaTime = Context.GetDeltaTimeSeconds();
	UFlowFieldSubsystem* FlowFieldSubsystem = GetWorld()->GetSubsystem<UFlowFieldSubsystem>();
	
	EntityQuery.ForEachEntityChunk(Context, [this, DeltaTime, FlowFieldSubsystem](FMassExecutionContext& IterContext)
	{
		auto Velocities = IterContext.GetMutableFragmentView<FMassVelocityFragment>();
		auto Transforms = IterContext.GetMutableFragmentView<FTransformFragment>();
		auto Gravities = IterContext.GetMutableFragmentView<FGravityFragment>();
		
		auto Heights = IterContext.GetFragmentView<FHeightFragment>();
	
		// Handle velocity objects
		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i) {
			const FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
				
			// Skip the gravity computation if already on the ground.
			if (FlowFieldSubsystem->GetGroundHeight() + (Heights[i].Height / 2.f) + 0.1f >= CurrentLocation.Z)
			{
				Velocities[i].Value.Z = 0;
				Gravities[i].AccumulatedVelocity = 0;
				IterContext.Defer().RemoveTag<FGravityTag>(IterContext.GetEntity(i));
				continue;
			}
			
			// Perform a trace (expensive)
			FVector TargetPos = CurrentLocation + (Velocities[i].Value * DeltaTime);	
			FVector TraceStart = TargetPos + FVector(0, 0, -Gravities[i].AccumulatedVelocity); // Prevent clipping into the ground.
			FVector TraceEnd = TargetPos + FVector(0, 0, -(Heights[i].Height / 2));
			FCollisionObjectQueryParams QueryParams;
			for (const auto ObjectType : WALL_COLLISION) QueryParams.AddObjectTypesToQuery(ObjectType);
			FHitResult Hit;
			if (GetWorld()->LineTraceSingleByObjectType(Hit, TraceStart, TraceEnd, QueryParams))
			{
				// Snap to the ground if hit.
				float GroundZ = Hit.ImpactPoint.Z + (Heights[i].Height / 2.f);
	    
				// Update the transform directly (since velocity doesn't handle collisions)
				FTransform UpdatedTransform = Transforms[i].GetTransform();
				UpdatedTransform.SetLocation(FVector(CurrentLocation.X, CurrentLocation.Y, GroundZ));
				Transforms[i].SetTransform(UpdatedTransform);
				Velocities[i].Value.Z = 0;
				Gravities[i].AccumulatedVelocity = 0;
				IterContext.Defer().RemoveTag<FGravityTag>(IterContext.GetEntity(i));
				continue;
			}
				
			// Keep adding gravity force if no ground hit.
			Gravities[i].AccumulatedVelocity += GRAVITY * DeltaTime;
			Velocities[i].Value.Z += Gravities[i].AccumulatedVelocity;
		}
	});
}
