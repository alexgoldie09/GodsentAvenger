// Alex Goldberg rights reserved

#include "AI/EnvQueryContext_AldegonSummons.h"
#include "AIController.h"
#include "Components/Aldegon/GodsentAldegonComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "GameFramework/Pawn.h"

void UEnvQueryContext_AldegonSummons::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	UObject* QuerierObject = QueryInstance.Owner.Get();

	// A behaviour tree runs its queries with the pawn as querier; accept the controller too
	const APawn* QuerierPawn = Cast<APawn>(QuerierObject);

	if (const AAIController* AIController = Cast<AAIController>(QuerierObject))
		QuerierPawn = AIController->GetPawn();

	const UGodsentAldegonComponent* Aldegon = QuerierPawn ? QuerierPawn->FindComponentByClass<UGodsentAldegonComponent>() : nullptr;

	if (!Aldegon)
		return;

	TArray<AActor*> Summons;
	Aldegon->GetLivingSummons(Summons);

	TArray<const AActor*> ContextActors;

	for (const AActor* Summon : Summons)
		ContextActors.Add(Summon);

	UEnvQueryItemType_Actor::SetContextHelper(ContextData, ContextActors);
}
