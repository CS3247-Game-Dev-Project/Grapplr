// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "UMassHealthProcessor.generated.h"

/**
 * Handles health logic.
 */
UCLASS()
class CS3247_GROUP2_API UMassHealthProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassHealthProcessor();
	
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
