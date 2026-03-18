#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IEnemyDeath.generated.h"

UINTERFACE(Blueprintable)
class CS3247_GROUP2_API UEnemyDeath : public UInterface
{
	GENERATED_BODY()
};

class CS3247_GROUP2_API IEnemyDeath
{
	GENERATED_BODY()

public:
	/**
	 * This function is called whenever an enemy is killed.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mass|Enemy")
	void OnEnemyKilled(int EnemyKillCount);
};
