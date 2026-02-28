#include "MassEnemyTrait.h"
#include "MassCommonFragments.h"
#include "MassNavigationFragments.h"
#include "MassEntityTemplateRegistry.h"
#include "MassStateTreeFragments.h"
#include "CS3247_Group2/Mass/Fragments/FEnemyFragments.h"
#include "CS3247_Group2/Mass/Processors/UMassMovementProcessor.h"

void UMassEnemyTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	// Add our custom health, and enemy stats.
	BuildContext.AddFragment<FHealthFragment>();
}
