#include "MassEnemyTrait.h"
#include "MassCommonFragments.h"
#include "MassNavigationFragments.h"
#include "MassEntityTemplateRegistry.h"
#include "Steering/MassSteeringFragments.h"
#include "CS3247_Group2/Mass/Fragments/FEnemyTag.h"
#include "CS3247_Group2/Mass/Fragments/FHealthFragment.h"

void UMassEnemyTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	// Add our custom health and identity
	BuildContext.AddFragment<FHealthFragment>();
	BuildContext.AddTag<FEnemyTag>();

	// Add built-in Navigation fragments (Required for Navmesh movement)
	BuildContext.AddFragment<FTransformFragment>();
	BuildContext.AddFragment<FMassMoveTargetFragment>();
	BuildContext.AddFragment<FMassSteeringFragment>();
}
