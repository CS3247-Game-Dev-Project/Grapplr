#pragma once

#include "MassEntityTraitBase.h"
#include "MassEnemyTrait.generated.h"

UCLASS()
class CS3247_GROUP2_API UMassEnemyTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()
	
protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
