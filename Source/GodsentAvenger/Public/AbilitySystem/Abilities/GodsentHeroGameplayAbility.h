// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GodsentGameplayAbility.h"
#include "GodsentHeroGameplayAbility.generated.h"

class AGodsentHeroCharacter;
class AGodsentHeroController;

/**
 * Base class for the hero's abilities.
 * GetHeroCombatComponentFromActorInfo and CreateHeroDamageEffectSpecHandle come back in stage 2.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentHeroGameplayAbility : public UGodsentGameplayAbility
{
	GENERATED_BODY()

public:
	/// Gets (and caches) the hero character that owns this ability
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	AGodsentHeroCharacter* GetHeroCharacterFromActorInfo();

	/// Gets (and caches) the hero's player controller
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	AGodsentHeroController* GetHeroControllerFromActorInfo();

	/// Gets the total and remaining time of the cooldown effect carrying InCooldownTag.
	/// Returns true while the cooldown is still running.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Ability")
	bool GetAbilityRemainingCooldownByTag(FGameplayTag InCooldownTag, float& TotalCooldownTime, float& RemainingCooldownTime);

	/// The hero's AttackSpeed attribute, for the Rate pin of Play Montage And Wait (1 = normal speed)
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	float GetAttackSpeedMontageRate() const;

private:
	/// Cached hero character, to avoid casting on every call
	TWeakObjectPtr<AGodsentHeroCharacter> CachedHeroCharacter;

	/// Cached hero controller, to avoid casting on every call
	TWeakObjectPtr<AGodsentHeroController> CachedHeroController;
};
