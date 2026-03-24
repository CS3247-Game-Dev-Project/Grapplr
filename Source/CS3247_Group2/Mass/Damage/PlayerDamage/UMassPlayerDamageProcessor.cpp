#include "UMassPlayerDamageProcessor.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "CS3247_Group2/Mass/Damage/FDamageFragments.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"

#include <atomic>

UMassPlayerDamageProcessor::UMassPlayerDamageProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PostPhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Tasks;

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassPlayerDamageProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FDamageFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassPlayerDamageProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());

	FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
	std::atomic<float> TotalDamage = 0.0f;
	
	EntityQuery.ForEachEntityChunk(Context, [PlayerLocation, &TotalDamage](const FMassExecutionContext& IterContext)
	{
		const auto Damages = IterContext.GetFragmentView<FDamageFragment>();
		const auto Transforms = IterContext.GetFragmentView<FTransformFragment>();
		
		float ChunkDamage = 0.0f;
       
		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			// Optimization: Use DistSquared to avoid expensive Square Root
			const float DistSq = FVector::DistSquared(PlayerLocation, Transforms[i].GetTransform().GetLocation()); 
			if (DistSq <= FMath::Square(Damages[i].AttackRange)) ChunkDamage += Damages[i].Damage;
		}
		
		TotalDamage += ChunkDamage;
	});
	
	if (TotalDamage > 0.0f)
	{
		const float DeltaTime = Context.GetDeltaTimeSeconds();
		const float Dmg = TotalDamage * DeltaTime;
		AsyncTask(ENamedThreads::GameThread, [this, Dmg]()
		{
			if (auto* Subsystem = GetWorld()->GetSubsystem<UPlayerDataSubsystem>())
			{
				Subsystem->AddPlayerDamage(Dmg);
			}
		});

	}
}
