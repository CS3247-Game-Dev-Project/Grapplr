#pragma once

#include "UEnemyMovementSubsystem.generated.h"

UCLASS()
class CS3247_GROUP2_API UEnemyMovementSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, Category = "Mass|Enemy")
	float GlobalMovementSpeedMultiplier = 1.0f;
};
