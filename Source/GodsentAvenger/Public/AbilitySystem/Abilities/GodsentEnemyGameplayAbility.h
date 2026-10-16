// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "ScalableFloat.h"
#include "AbilitySystem/Abilities/GodsentGameplayAbility.h"
#include "GodsentEnemyGameplayAbility.generated.h"

class AGodsentEnemyCharacter;
class UEnemyCombatComponent;

/**
 * Base class for enemy abilities (attacks, hit react, death).
 */
UCLASS()
class GODSENTAVENGER_API UGodsentEnemyGameplayAbility : public UGodsentGameplayAbility
{
	GENERATED_BODY()

public:
	/// Gets (and caches) the enemy character that owns this ability
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	AGodsentEnemyCharacter* GetEnemyCharacterFromActorInfo();

	/// Gets the enemy's combat component
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	UEnemyCombatComponent* GetEnemyCombatComponentFromActorInfo();

	/// Builds the damage spec for an enemy attack from a per-level damage value
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	FGameplayEffectSpecHandle CreateEnemyDamageEffectSpecHandle(TSubclassOf<UGameplayEffect> EffectClass, const FScalableFloat& InDamageScalableFloat);

private:
	TWeakObjectPtr<AGodsentEnemyCharacter> CachedGodsentEnemyCharacter;
};
