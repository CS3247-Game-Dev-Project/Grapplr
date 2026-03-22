#include "USpatialGridUpdateProcessor.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "UMassSpatialGridSubsystem.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"

USpatialGridUpdateProcessor::USpatialGridUpdateProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Avoidance;
	ExecutionOrder.ExecuteBefore.Add(TEXT("SpatialGridAvoidanceProcessor"));

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void USpatialGridUpdateProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSpatialGridAvoidanceFragment>(EMassFragmentAccess::None);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void USpatialGridUpdateProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	auto SpatialGridSubsystem = GetWorld()->GetSubsystem<UMassSpatialGridSubsystem>();
	SpatialGridSubsystem->Reset();
	
	// Cache the debug flag.
	bool bShowDebug = false;
#if WITH_EDITOR
	bShowDebug = CVarShowMassSpatialGrid.GetValueOnAnyThread() > 0;
#endif
	
	// Include player in avoidance calculations.
	const auto PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
	const int32 PlayerGridIdx = UMassSpatialGridSubsystem::GetGridIndex(PlayerLocation);
	SpatialGridSubsystem->EntityLocations[UMassSpatialGridSubsystem::PLAYER_INDEX] = PlayerLocation;
	SpatialGridSubsystem->NextEntity[UMassSpatialGridSubsystem::PLAYER_INDEX] = SpatialGridSubsystem->CellHeads[PlayerGridIdx];
	SpatialGridSubsystem->CellHeads[PlayerGridIdx] = UMassSpatialGridSubsystem::PLAYER_INDEX;

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& IterContext)
	{
		auto Transforms = IterContext.GetFragmentView<FTransformFragment>();
		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			int32 EntityIdx = IterContext.GetEntity(i).Index;
			FVector Loc = Transforms[i].GetTransform().GetLocation();
        
			// Cache location for the next pass
			SpatialGridSubsystem->EntityLocations[EntityIdx] = Loc;

			int32 GridIdx = UMassSpatialGridSubsystem::GetGridIndex(Loc);
			SpatialGridSubsystem->NextEntity[EntityIdx] = SpatialGridSubsystem->CellHeads[GridIdx];
			SpatialGridSubsystem->CellHeads[GridIdx] = EntityIdx;
		
#if WITH_EDITOR
			if (bShowDebug) {
				// Draw a wireframe box for every cell that contains at least one enemy
				DrawDebugBox(GetWorld(), UMassSpatialGridSubsystem::GetCellCenterFromPos(Loc), FVector(SpatialConfig::CellSize * 0.5f),
							 FColor::Green, false, -1, 0, 2.0f);
			}
#endif
			
		}
	});
}
