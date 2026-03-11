#pragma once

#include "MassProcessor.h"
#include "UMassEnemyDamageProcessor.generated.h"

/**
 * Handles enemy health taking damage logic.
 */
UCLASS()
class CS3247_GROUP2_API UMassEnemyDamageProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassEnemyDamageProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
