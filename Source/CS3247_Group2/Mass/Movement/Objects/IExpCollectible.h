#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IExpCollectible.generated.h"

UINTERFACE(Blueprintable)
class CS3247_GROUP2_API UExpCollectible : public UInterface
{
	GENERATED_BODY()
};

class CS3247_GROUP2_API IExpCollectible
{
	GENERATED_BODY()

public:
	/**
	 * DEPRECATED: Use PlayerDataSubsystem instead to avoid unsafe nullptr crash.
	 * 
	 * This function is called whenever an Exp Drop is collected (and removed).
	 * The player Exp stat should be updated afterward.
	 * @param Amount amount of exp collected for this instance
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mass|Exp")
	void OnExperienceCollected(int Amount);
};
