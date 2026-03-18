#pragma once
#include "UEnemyGlobalSubsystem.generated.h"

UCLASS()
class CS3247_GROUP2_API UEnemyGlobalSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, Category = "Mass|Enemy")
	float GlobalMovementSpeedMultiplier = 1.0f;
};
