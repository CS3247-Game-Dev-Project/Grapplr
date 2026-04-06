#pragma once

#include "UDamageNumberSubsystem.generated.h"

// Listen to these broadcast events in blueprints.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamageEmit,const TArray<FVector>&, Locations, const TArray<float>&, Amounts, const TArray<bool>&, Crits);

/** 
 * Handles damage number emitting to blueprints, blueprints can then forward them or render them via actors, etc.
 */
UCLASS()
class CS3247_GROUP2_API UDamageNumberSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, Category = "Mass|Enemy");
	float TotalDamageEmitted = 0;
	
	UPROPERTY(BlueprintAssignable, Category = "Mass|Enemy")
	FOnDamageEmit OnDamageEmit;
	
	void EmitDamageNumbers(const TArray<FVector>& Locations, const TArray<float>& Amounts, const TArray<bool>& Crits)
	{
		for (auto Amount : Amounts)
		{
			TotalDamageEmitted += Amount;
		}
		if (!IsValid(this) || !GetWorld() || GetWorld()->IsBeingCleanedUp()) return;
		if (!OnDamageEmit.IsBound()) return;
		OnDamageEmit.Broadcast(Locations, Amounts, Crits);
	}
	
	virtual void Deinitialize() override
	{
		// Must unbind every listen to be safe, otherwise a null call to delegate will crash the editor.
		// This is only safe because the subsystem only runs in the game world, and not the editor viewport.
		OnDamageEmit.Clear();
		
		Super::Deinitialize();
	}
};