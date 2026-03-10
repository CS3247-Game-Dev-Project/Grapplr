#pragma once
#include "CS3247_Group2/Mass/Actors/ADamageNumberManager.h"
#include "UDamageManagerSubsystem.generated.h"

UCLASS()
class CS3247_GROUP2_API UDamageManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UPROPERTY(Transient)
	ADamageNumberManager* DamageManager; 
};