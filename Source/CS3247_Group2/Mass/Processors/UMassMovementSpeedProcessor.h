#pragma once
#include "MassProcessor.h"
#include "UMassMovementSpeedProcessor.generated.h"

UCLASS()
class CS3247_GROUP2_API UMassMovementSpeedProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassMovementSpeedProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
