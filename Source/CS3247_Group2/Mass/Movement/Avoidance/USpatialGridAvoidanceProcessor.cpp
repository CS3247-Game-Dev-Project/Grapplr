#include "USpatialGridAvoidanceProcessor.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassNavigationFragments.h"
#include "UMassSpatialGridSubsystem.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"

USpatialGridAvoidanceProcessor::USpatialGridAvoidanceProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Avoidance;
	ExecutionOrder.ExecuteAfter.Add(TEXT("SpatialGridUpdateProcessor"));

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void USpatialGridAvoidanceProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FSpatialGridAvoidanceFragment>(EMassFragmentAccess::ReadOnly);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void USpatialGridAvoidanceProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	auto SpatialGridSubsystem = GetWorld()->GetSubsystem<UMassSpatialGridSubsystem>();
	FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
	constexpr float AVOIDANCE_WEIGHT = 0.9f; // TODO: customizable
	constexpr float SIDE_STEER_STRENGTH = 10;
	constexpr float SPRING_STIFFNESS = 10; // TODO: customizable, stronger => more stronger avoidance
	
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& IterContext)
	{
		const auto MoveTargets = IterContext.GetMutableFragmentView<FMassMoveTargetFragment>();
		const auto Transforms = IterContext.GetFragmentView<FTransformFragment>();
		const auto Avoidances = IterContext.GetFragmentView<FSpatialGridAvoidanceFragment>();

		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			FVector MyPos = Transforms[i].GetTransform().GetLocation();
			auto& MoveTarget = MoveTargets[i];
			int32 MyEntityIdx = IterContext.GetEntity(i).Index; 
			
			// Use the PREVIOUS Forward as the base for steering
			// This preserves the direction set by pathfinding/pursuit
			FVector BaseDirection = MoveTarget.Forward;
			
			FVector SeparationForce = FVector::ZeroVector;
			FVector SideSteerForce = FVector::ZeroVector;
			float RadiusSq = FMath::Square(Avoidances[i].AvoidanceSpaceRadius);
			
			// Loop 27 cells
			for (int z = -1; z <= 1; ++z) {
				for (int y = -1; y <= 1; ++y) {
					for (int x = -1; x <= 1; ++x) {
						
						int32 TargetIdx = UMassSpatialGridSubsystem::GetGridIndex(MyPos + FVector(x * SpatialConfig::CellSize, y * SpatialConfig::CellSize, z * SpatialConfig::CellSize));
						for  (int32 OtherIdx = SpatialGridSubsystem->CellHeads[TargetIdx]; OtherIdx != -1; OtherIdx = SpatialGridSubsystem->NextEntity[OtherIdx]) 
						{
							if (OtherIdx == MyEntityIdx) continue;
							
							FVector OtherPos = SpatialGridSubsystem->GetLocationFromIdx(OtherIdx);
							FVector ToOther = OtherPos - MyPos;
							float DistSq = ToOther.SizeSquared();
							if (DistSq < RadiusSq)
							{
								ToOther.Normalize();
								float Dist = FMath::Sqrt(DistSq);
								
								// Side Steering logic:
								// 1. Check: Is this neighbor actually in front of me?
								// Dot Product > 0.5 means they are roughly in our 45-degree forward cone
								float ForwardBlockage = FVector::DotProduct(BaseDirection, ToOther);
								if (ForwardBlockage > 0.5f)
								{
									// // 2. Calculate a "Right" vector relative to our path
									// FVector RightVector = FVector::CrossProduct(BaseDirection, FVector::UpVector);
						   //          
									// // 3. Decide to steer Left or Right based on where the neighbor is
									// float SideCheck = FVector::DotProduct(RightVector, ToOther);
									// FVector SteerDir = (SideCheck > 0.f) ? -RightVector : RightVector;
									//
									// // 4. Add weight based on how close they are
									// float Proximity = 1.0f - (Dist / Avoidances[i].AvoidanceSpaceRadius);
									// SideSteerForce += SteerDir * Proximity;
									
									// inside the 'if (ForwardBlockage > 0.5f)' block
									FVector RightVector = FVector::CrossProduct(BaseDirection, FVector::UpVector);
									float SideCheck = FVector::DotProduct(RightVector, ToOther);

									// Pick the "clearer" side
									FVector SteerDir = (SideCheck > 0.f) ? -RightVector : RightVector;

									// IMPORTANT: The closer they are, the more we TANGENT, the less we FORWARD
									float DangerFactor = FMath::Clamp(1.0f - (Dist / Avoidances[i].AvoidanceSpaceRadius), 0.0f, 1.0f);

									// This is the secret: We don't just ADD side force; we REDUCE forward force 
									// so they actually turn instead of just sliding sideways.
									SideSteerForce += SteerDir * DangerFactor * SIDE_STEER_STRENGTH;
								}
								
								FVector PushDir = (Dist < 0.01f) ? FVector(1,0,0) : (MyPos - OtherPos) / Dist;
								SeparationForce += PushDir * (Avoidances[i].AvoidanceSpaceRadius - Dist) * SPRING_STIFFNESS; 
							}
						}
					}
				}
			}
			// TODO: clean up code and logic
			
			// 1. Get the separation nudge scaled by time
			SeparationForce *= IterContext.GetDeltaTimeSeconds();
			SideSteerForce *= IterContext.GetDeltaTimeSeconds();

			float BlockageAmount = SideSteerForce.Size(); // Normalized by your strength constant

			// 2. Interpolate between our "Path" and our "Avoidance"
			// If Blockage is high (1.0), we purely steer sideways. 
			// If Blockage is low (0.0), we purely move forward.
			FVector FinalDir = FMath::Lerp(BaseDirection, SideSteerForce.GetSafeNormal(), FMath::Min(BlockageAmount, 0.8f));
			
			// 4. Calculate Proximity Scaler using the existing DistanceToGoal
			float ProximityToPlayer = FMath::Clamp(MoveTarget.DistanceToGoal / 1000.f, 0.2f, 1.0f);
			float FinalAvoidanceWeight = AVOIDANCE_WEIGHT * (1.0f / ProximityToPlayer);

			// 5. NUDGE the existing direction rather than replacing it
			// We add the avoidance force to the direction already decided by other processors
			FVector NewSteeredDirection = FinalDir + (SeparationForce * FinalAvoidanceWeight) + SideSteerForce;

			// 6. Update the Forward with the blended result
			if (!NewSteeredDirection.IsNearlyZero())
			{
				MoveTarget.Forward = NewSteeredDirection.GetSafeNormal();
			} else
			{
				MoveTarget.Forward = (PlayerLocation - Transforms[i].GetTransform().GetLocation()).GetSafeNormal();
			}

			// If we are very close to the player, we dampen the base direction 
			// so the avoidance force can take over and spread us out.
			// float ArrivalWeight = FMath::Clamp(MoveTarget.DistanceToGoal / 200.f, 0.0f, 1.0f);
			// FVector SteeredDirection = (BaseDirection * ArrivalWeight) + (SeparationForce * AVOIDANCE_WEIGHT);
			//
			// if (!SteeredDirection.IsNearlyZero()) {
			// 	MoveTarget.Forward = SteeredDirection.GetSafeNormal();
			// }
		}
	});
}
