#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MassAgentComponent.h"
#include "UMassDamageHandler.generated.h"

UCLASS()
class CS3247_GROUP2_API UMassDamageHandler : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** * Applies damage to a specific Mass Entity safely from Blueprint.
	 * @param WorldContextObject - Used to find the Mass Subsystem
	 * @param AgentComponent - The mass agent component of the entity to damage
	 * @param DamageAmount - How much health to subtract
	 */
	UFUNCTION(BlueprintCallable, Category = "Mass|Enemy", meta = (WorldContext = "WorldContextObject"))
	static void ApplyDamageToEntity(const UObject* WorldContextObject, UMassAgentComponent* AgentComponent,
	                                float DamageAmount);
};
