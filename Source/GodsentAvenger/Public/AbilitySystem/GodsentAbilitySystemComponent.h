// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GodsentTypes/GodsentStructTypes.h"
#include "GodsentAbilitySystemComponent.generated.h"

/**
 * Routes input tags to abilities and grants hero ability sets.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/// Activates (or toggles off) every granted ability bound to the pressed input tag
	void OnAbilityInputPressed(const FGameplayTag& InputTag);

	/// Cancels active MustBeHeld abilities bound to the released input tag
	void OnAbilityInputReleased(const FGameplayTag& InputTag);

	/// Grants a list of hero abilities at a level, binding each to its input tag.
	/// Replaces the tutorial's GrantHeroWeaponAbilities so the weapon and the skill tree can share it.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Ability", meta = (ApplyLevel = "1"))
	void GrantHeroAbilitySets(const TArray<FGodsentHeroAbilitySet>& InAbilitySets, int32 ApplyLevel, TArray<FGameplayAbilitySpecHandle>& OutGrantedAbilitySpecHandles);

	/// Removes previously granted abilities and empties the handle array
	UFUNCTION(BlueprintCallable, Category = "Godsent|Ability")
	void RemoveGrantedAbilities(UPARAM(ref) TArray<FGameplayAbilitySpecHandle>& InGrantedAbilitySpecHandlesToRemove);

	/// Activates a random granted ability matching the tag (used by enemy AI)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Ability")
	bool TryActivateAbilityByTag(FGameplayTag AbilityTagToActivate);
};
