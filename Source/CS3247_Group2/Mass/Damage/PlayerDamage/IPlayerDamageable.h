#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IPlayerDamageable.generated.h"

UINTERFACE(Blueprintable)
class CS3247_GROUP2_API UPlayerDamageable : public UInterface
{
	GENERATED_BODY()
};

class CS3247_GROUP2_API IPlayerDamageable
{
	GENERATED_BODY()

public:
	/**
	 * DEPRECATED: Use PlayerDataSubsystem instead to avoid unsafe nullptr crash.
	 * 
	 * This function is called whenever the player is damaged.
	 * The player's health stat should be updated afterward.
	 * @param Amount amount of damage taken by the player for this instance
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mass|Player")
	void OnPlayerDamaged(float Amount);
};
