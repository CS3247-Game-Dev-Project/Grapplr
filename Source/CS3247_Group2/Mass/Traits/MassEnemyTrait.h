#pragma once

#include "MassEntityTraitBase.h"
#include "MassEnemyTrait.generated.h"

UCLASS()
class UMassEnemyTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()
	
protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
