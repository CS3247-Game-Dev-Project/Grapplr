#include "MassEnemyTrait.h"
#include "MassEntityTemplateRegistry.h"
#include "CS3247_Group2/Mass/Damage/FDamageFragments.h"
#include "CS3247_Group2/Mass/Damage/FHealthFragments.h"
#include "CS3247_Group2/Mass/EnemyDeath/FEnemyDrops.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"

void UMassEnemyTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	// Add our custom health, and enemy stats.
	// This groups all the fragments together, should refactor in future if need to split functionality.
	BuildContext.AddFragment<FHealthFragment>();
	BuildContext.AddFragment<FMovementSpeedFragment>();
	BuildContext.AddFragment<FDamageFragment>();
	BuildContext.AddFragment<FDamageAccumulatorFragment>();
	BuildContext.AddFragment<FDamageDisplayFragment>();
	BuildContext.AddFragment<FDropStatsFragment>();
	BuildContext.AddFragment<FSpatialGridAvoidanceFragment>();
	BuildContext.AddFragment<FMassDriftFragment>();
	BuildContext.AddFragment<FHeightFragment>();
}
