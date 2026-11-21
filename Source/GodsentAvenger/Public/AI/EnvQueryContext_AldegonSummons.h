// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_AldegonSummons.generated.h"

/**
 * EQS context: every add and totem Aldegon has alive, so new summons and landing spots can keep their distance.
 * Empty if the querier isn't Aldegon (or has nothing out).
 */
UCLASS(meta = (DisplayName = "Aldegon Summons"))
class GODSENTAVENGER_API UEnvQueryContext_AldegonSummons : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
