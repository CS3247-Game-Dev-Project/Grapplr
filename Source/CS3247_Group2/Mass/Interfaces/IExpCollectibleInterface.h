#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IExpCollectibleInterface.generated.h"

UINTERFACE(Blueprintable)
class CS3247_GROUP2_API UExpCollectibleInterface : public UInterface
{
	GENERATED_BODY()
};

class CS3247_GROUP2_API IExpCollectibleInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mass|Collectibles")
	void OnExperienceCollected(int Amount);
};
