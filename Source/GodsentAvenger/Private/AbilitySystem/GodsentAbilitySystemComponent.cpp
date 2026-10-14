// Alex Goldberg rights reserved

#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "GodsentGameplayTags.h"
#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility.h"

/// Activates (or toggles off) every granted ability bound to the pressed input tag
void UGodsentAbilitySystemComponent::OnAbilityInputPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
		return;

	// Defers any ability list changes (e.g. an OnGiven ability clearing itself) until the loop finishes
	ABILITYLIST_SCOPE_LOCK();

	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			continue;

		// Toggleable abilities cancel when pressed again while active
		if (InputTag.MatchesTag(GodsentGameplayTags::InputTag_Toggleable) && AbilitySpec.IsActive())
			CancelAbilityHandle(AbilitySpec.Handle);
		else
			TryActivateAbility(AbilitySpec.Handle);
	}
}

/// Cancels active MustBeHeld abilities bound to the released input tag
void UGodsentAbilitySystemComponent::OnAbilityInputReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid() || !InputTag.MatchesTag(GodsentGameplayTags::InputTag_MustBeHeld))
		return;

	ABILITYLIST_SCOPE_LOCK();

	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag) && AbilitySpec.IsActive())
			CancelAbilityHandle(AbilitySpec.Handle);
	}
}

/// Grants a list of hero abilities at a level, binding each to its input tag
void UGodsentAbilitySystemComponent::GrantHeroAbilitySets(const TArray<FGodsentHeroAbilitySet>& InAbilitySets,
	int32 ApplyLevel, TArray<FGameplayAbilitySpecHandle>& OutGrantedAbilitySpecHandles)
{
	for (const FGodsentHeroAbilitySet& AbilitySet : InAbilitySets)
	{
		if (!AbilitySet.IsValid())
			continue;

		FGameplayAbilitySpec AbilitySpec(AbilitySet.AbilityToGrant);
		AbilitySpec.SourceObject = GetAvatarActor();
		AbilitySpec.Level = ApplyLevel;
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilitySet.InputTag);

		OutGrantedAbilitySpecHandles.AddUnique(GiveAbility(AbilitySpec));
	}
}

/// Removes previously granted abilities and empties the handle array
void UGodsentAbilitySystemComponent::RemoveGrantedAbilities(TArray<FGameplayAbilitySpecHandle>& InGrantedAbilitySpecHandlesToRemove)
{
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : InGrantedAbilitySpecHandlesToRemove)
	{
		if (AbilitySpecHandle.IsValid())
			ClearAbility(AbilitySpecHandle);
	}

	InGrantedAbilitySpecHandlesToRemove.Empty();
}

/// Activates a random granted ability matching the tag (used by enemy AI)
bool UGodsentAbilitySystemComponent::TryActivateAbilityByTag(FGameplayTag AbilityTagToActivate)
{
	check(AbilityTagToActivate.IsValid());

	TArray<FGameplayAbilitySpec*> FoundAbilitySpecs;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(AbilityTagToActivate.GetSingleTagContainer(), FoundAbilitySpecs);

	if (FoundAbilitySpecs.IsEmpty())
		return false;

	const int32 RandomAbilityIndex = FMath::RandRange(0, FoundAbilitySpecs.Num() - 1);
	FGameplayAbilitySpec* SpecToActivate = FoundAbilitySpecs[RandomAbilityIndex];
	check(SpecToActivate);

	if (!SpecToActivate->IsActive())
		return TryActivateAbility(SpecToActivate->Handle);

	return false;
}
