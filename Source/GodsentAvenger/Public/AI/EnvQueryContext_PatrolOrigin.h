// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_PatrolOrigin.generated.h"

/**
 * EQS context: the enemy's home (where it was placed or spawned), so patrol points are picked around a fixed spot
 * instead of around wherever the enemy wandered to. Falls back to the querier's location if it has no Godsent AI controller.
 */
UCLASS(meta = (DisplayName = "Patrol Origin (Home Location)"))
class GODSENTAVENGER_API UEnvQueryContext_PatrolOrigin : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
