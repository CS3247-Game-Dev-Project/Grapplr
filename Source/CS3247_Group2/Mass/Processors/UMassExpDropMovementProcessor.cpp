#include "UMassExpDropMovementProcessor.h"
#include "MassExecutionContext.h"
#include "MassCommonTypes.h"
#include "MassNavigationFragments.h"
#include "CS3247_Group2/Mass/Structs/FEnemyDrops.h"
#include "MassMovementFragments.h"
#include "CS3247_Group2/Mass/Interfaces/IExpCollectibleInterface.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

UMassExpDropMovementProcessor::UMassExpDropMovementProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassExpDropMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.Initialize(EntityManager);

	EntityQuery.AddRequirement<FExpDropFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());

	// Get Player
	Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
}

void UMassExpDropMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());

	if (!Player)
	{
		return;
	}
	
	FVector PlayerLocation = Player->GetActorLocation();
	const float DeltaTime = Context.GetDeltaTimeSeconds();
	
	// Iterate through all entities
	EntityQuery.ForEachEntityChunk(Context, [this, PlayerLocation, DeltaTime](FMassExecutionContext& IterContext)
	{
		auto Velocities = IterContext.GetMutableFragmentView<FMassVelocityFragment>();
		auto Transforms = IterContext.GetMutableFragmentView<FTransformFragment>();
		auto ExpDrops = IterContext.GetFragmentView<FExpDropFragment>();

		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			const FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
			
			// Only move if within the detection radius
			if (float DistanceToPlayer = FVector::Dist(PlayerLocation, CurrentLocation); DistanceToPlayer <= MaxDetectionRadius)
			{
				// Calculate "Closer = Faster" factor (ranges from 0.0 at edge to 1.0 at player)
				float SpeedFactor = 1.0f - (DistanceToPlayer / MaxDetectionRadius);
				Velocities[i].Value = (PlayerLocation - CurrentLocation).GetSafeNormal() * (BaseMaxSpeed * SpeedFactor);
				
				if (DistanceToPlayer < 1.f) {
					if (Player->Implements<UExpCollectibleInterface>())
					{
						// Use the 'Execute_' static wrapper to call the function
						IExpCollectibleInterface::Execute_OnExperienceCollected(Player, ExpDrops[i].ExperienceAmount);
					}
					IterContext.Defer().DestroyEntity(IterContext.GetEntity(i));	
				}
			} else
			{	
				// Simulate gravity
				FVector TargetPos = CurrentLocation + (Velocities[i].Value * DeltaTime);	
				FVector TraceStart = TargetPos + FVector(0, 0, 500.f); 
				FVector TraceEnd = TargetPos + FVector(0, 0, -5.f);
				FCollisionQueryParams Params;
				Params.AddIgnoredActor(Player);
				if (FHitResult Hit; GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
				{
					// Snap the Height
					float GroundZ = Hit.ImpactPoint.Z;
	    
					// Update the Transform directly (since Velocity doesn't handle collisions)
					FTransform UpdatedTransform = Transforms[i].GetTransform();
					UpdatedTransform.SetLocation(FVector(TargetPos.X, TargetPos.Y, GroundZ));
					Transforms[i].SetTransform(UpdatedTransform);
					Velocities[i].Value = FVector::ZeroVector;
				} else
				{
					Velocities[i].Value += Gravity * DeltaTime;
				}
			}
		}
	});
}
