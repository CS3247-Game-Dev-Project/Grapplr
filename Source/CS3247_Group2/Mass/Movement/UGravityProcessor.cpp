#include "UGravityProcessor.h"
#include "UMassMovementSpeedProcessor.h"
#include "FMovementFragments.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassCommonTypes.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "CS3247_Group2/Mass/Constants.h"
#include "CS3247_Group2/Mass/EnemyDeath/FEnemyDrops.h"
#include "FlowField/UFlowFieldSubsystem.h"

UGravityProcessor::UGravityProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::DuringPhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::ApplyForces;
	ExecutionOrder.ExecuteAfter.Add(TEXT("UMassExpDropMovementProcessor"));
	ExecutionOrder.ExecuteAfter.Add(TEXT("UMassMovementSpeedProcessor"));
	ExecutionOrder.ExecuteAfter.Add(TEXT("UMassSteerToMoveTargetProcessor"));

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UGravityProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FGravityFragment>(EMassFragmentAccess::ReadWrite);
	
	EntityQuery.AddRequirement<FHeightFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FGravityTag>(EMassFragmentPresence::All);
	
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UGravityProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	const float DeltaTime = Context.GetDeltaTimeSeconds();
	UFlowFieldSubsystem* FlowFieldSubsystem = GetWorld()->GetSubsystem<UFlowFieldSubsystem>();
	
	// Iterate through all entities
	EntityQuery.ForEachEntityChunk(Context, [this, DeltaTime, FlowFieldSubsystem](FMassExecutionContext& IterContext)
	{
		auto Velocities = IterContext.GetMutableFragmentView<FMassVelocityFragment>();
		auto Transforms = IterContext.GetMutableFragmentView<FTransformFragment>();
		auto Gravities = IterContext.GetMutableFragmentView<FGravityFragment>();
		auto Heights = IterContext.GetFragmentView<FHeightFragment>();
		const bool bHasExpDrop = IterContext.DoesArchetypeHaveFragment<FExpDropFragment>();
		
		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			const FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
			
			// Perform a trace (expensive)
			FVector TargetPos = CurrentLocation + (Velocities[i].Value * DeltaTime);	
			FVector TraceStart = TargetPos + FVector(0, 0, Heights[i].Height / 2); // Prevent clipping into the ground.
			FVector TraceEnd = TargetPos + FVector(0, 0, -(Heights[i].Height / 2));
			FCollisionQueryParams Params;
			if (FHitResult Hit; GetWorld()->LineTraceSingleByObjectType(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
			{
				// Snap to the ground if hit.
				float GroundZ = Hit.ImpactPoint.Z + (Heights[i].Height / 2.f);
	    
				// Update the transform directly (since velocity doesn't handle collisions)
				FTransform UpdatedTransform = Transforms[i].GetTransform();
				UpdatedTransform.SetLocation(FVector(TargetPos.X, TargetPos.Y, GroundZ));
				Transforms[i].SetTransform(UpdatedTransform);
				Velocities[i].Value.Z = 0;
				Gravities[i].AccumulatedVelocity = 0;
				
				// Remove the gravity tag once stationary (only if unlike to change)
				if (bHasExpDrop || !(FlowFieldSubsystem->GetGroundHeight() + Heights[i].Height / 2.f < CurrentLocation.Z))
				{
					IterContext.Defer().RemoveTag<FGravityTag>(IterContext.GetEntity(i));
				}
			} else {
				// Simulate gravity.
				Gravities[i].AccumulatedVelocity += GRAVITY * DeltaTime;
				Velocities[i].Value.Z +=  Gravities[i].AccumulatedVelocity;
			}
		}
	});
}
