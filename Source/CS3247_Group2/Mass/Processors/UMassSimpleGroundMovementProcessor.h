#pragma once

#include "MassProcessor.h"
#include "UMassSimpleGroundMovementProcessor.generated.h"

UCLASS()
class CS3247_GROUP2_API UMassSimpleGroundMovementProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassSimpleGroundMovementProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;

private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> Player = nullptr;
};
