#pragma once
#include "UMassAnimationComponent.generated.h"

UENUM(BlueprintType)
enum class EEnemyAnimationState : uint8
{
	Idle    UMETA(DisplayName = "Idle"), // UNUSED
	Walk    UMETA(DisplayName = "Walking"),
	Attack  UMETA(DisplayName = "Attacking"),
	Death   UMETA(DisplayName = "Dead")
};

UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class CS3247_GROUP2_API UMassAnimationComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	// The state tree writes to this via the mass actor fragment; Blueprints read from this
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mass Sync")
	EEnemyAnimationState CurrentAnimState = EEnemyAnimationState::Walk;
};
