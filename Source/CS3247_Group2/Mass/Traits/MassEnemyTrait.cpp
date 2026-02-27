#include "MassEnemyTrait.h"
#include "MassCommonFragments.h"
#include "MassNavigationFragments.h"
#include "MassEntityTemplateRegistry.h"
#include "Steering/MassSteeringFragments.h"
#include "CS3247_Group2/Mass/Fragments/FEnemyFragments.h"

void UMassEnemyTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	// Add our custom health, and enemy stats.
	BuildContext.AddFragment<FHealthFragment>();
}
