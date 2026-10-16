// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GEExecCalc_DamageTaken.generated.h"

/**
 * Damage = BaseDamage x combo bonus x source AttackPower / target DefensePower, written to DamageTaken.
 * Used by GE_Shared_DealDamage.
 */
UCLASS()
class GODSENTAVENGER_API UGEExecCalc_DamageTaken : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	/// Registers the source's AttackPower and the target's DefensePower and DamageTaken for capture
	UGEExecCalc_DamageTaken();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
