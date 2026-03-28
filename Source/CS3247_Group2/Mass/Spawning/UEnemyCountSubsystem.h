#pragma once

#include "UEnemyCountSubsystem.generated.h"

UCLASS()
class CS3247_GROUP2_API UEnemyCountSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	/** Tracks the total number of active enemies (spawned and not dead) */
	UPROPERTY(BluePrintReadOnly, Category="Mass|Enemy")
	int EnemyCount = 0;
};
