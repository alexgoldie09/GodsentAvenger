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

	/// Cancels active MustBeHeld abilities bound to the released input tag;
	/// tells any other active ability bound to it that the button came up (fires Wait Input Release)
	void OnAbilityInputReleased(const FGameplayTag& InputTag);

	/// Grants one hero ability at a level, bound to its input tag (used by the skill tree to unlock a skill)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Ability", meta = (ApplyLevel = "1"))
	FGameplayAbilitySpecHandle GrantHeroAbility(const FGodsentHeroAbilitySet& InAbilitySet, int32 ApplyLevel);

	/// Sets the level of every granted ability whose Asset Tags contain AbilityTag exactly
	/// (used by the skill tree to upgrade a skill, e.g. Hop -> Leap -> Graced Evasion). Returns false if none was found.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Ability")
	bool SetAbilityLevelByTag(FGameplayTag AbilityTag, int32 NewLevel);

	/// Gets the level of the first granted ability whose Asset Tags contain AbilityTag exactly, or 0 if it isn't granted
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	int32 GetAbilityLevelByTag(FGameplayTag AbilityTag) const;

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
	
	/// Re-sends every granted hero ability's icon and running cooldown to the HUD.
	/// Call it from whatever creates the overlay widget, after the widget exists (abilities granted earlier fired before anyone was listening).
	UFUNCTION(BlueprintCallable, Category = "Godsent|Ability")
	void BroadcastHeroAbilityUI();
};
