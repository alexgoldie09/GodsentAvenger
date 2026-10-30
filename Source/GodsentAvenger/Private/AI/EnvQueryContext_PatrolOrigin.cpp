// Alex Goldberg rights reserved

#include "AI/EnvQueryContext_PatrolOrigin.h"
#include "Controllers/GodsentAIController.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "GameFramework/Pawn.h"

void UEnvQueryContext_PatrolOrigin::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	UObject* QuerierObject = QueryInstance.Owner.Get();

	// A behaviour tree runs its queries with the pawn as querier; accept the controller too
	const AGodsentAIController* AIController = Cast<AGodsentAIController>(QuerierObject);
	const APawn* QuerierPawn = Cast<APawn>(QuerierObject);

	if (!AIController && QuerierPawn)
		AIController = Cast<AGodsentAIController>(QuerierPawn->GetController());

	if (AIController && AIController->HasHomeLocation())
	{
		UEnvQueryItemType_Point::SetContextHelper(ContextData, AIController->GetHomeLocation());
		return;
	}

	if (const AActor* QuerierActor = Cast<AActor>(QuerierObject))
		UEnvQueryItemType_Point::SetContextHelper(ContextData, QuerierActor->GetActorLocation());
}
