#pragma once

#include "MassProcessor.h"
#include "UMassPlayerDamageProcessor.generated.h"

/**
 * Damages the player based on distance to the player.
 */
UCLASS()
class CS3247_GROUP2_API UMassPlayerDamageProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassPlayerDamageProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
	
private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> Player = nullptr;
};
