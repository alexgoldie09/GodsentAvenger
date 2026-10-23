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

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			continue;

		// Marks the button as held, so Wait Input Release (e.g. the charged heal) knows to wait for it
		AbilitySpec.InputPressed = true;

		// Toggleable abilities cancel when pressed again while active
		if (InputTag.MatchesTag(GodsentGameplayTags::InputTag_Toggleable) && AbilitySpec.IsActive())
			CancelAbilityHandle(AbilitySpec.Handle);
		else
			TryActivateAbility(AbilitySpec.Handle);
	}
}

/// Cancels active MustBeHeld abilities bound to the released input tag.
/// Any other active ability bound to it is told the button came up, which fires Wait Input Release (charged heal).
void UGodsentAbilitySystemComponent::OnAbilityInputReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
		return;

	const bool bMustBeHeld = InputTag.MatchesTag(GodsentGameplayTags::InputTag_MustBeHeld);

	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			continue;

		AbilitySpec.InputPressed = false;

		if (!AbilitySpec.IsActive())
			continue;

		if (bMustBeHeld)
		{
			CancelAbilityHandle(AbilitySpec.Handle);
			continue;
		}

		AbilitySpecInputReleased(AbilitySpec);

		// The engine only sends this for its own InputID binding, so send it ourselves for tag-bound input.
		// The key must match the one Wait Input Release registered with (the active instance's activation key).
		const UGameplayAbility* PrimaryInstance = AbilitySpec.GetPrimaryInstance();
		const FPredictionKey ActivationPredictionKey = PrimaryInstance
			? PrimaryInstance->GetCurrentActivationInfo().GetActivationPredictionKey()
			: FPredictionKey();

		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, AbilitySpec.Handle, ActivationPredictionKey);
	}
}

/// Grants one hero ability at a level, bound to its input tag
FGameplayAbilitySpecHandle UGodsentAbilitySystemComponent::GrantHeroAbility(const FGodsentHeroAbilitySet& InAbilitySet, int32 ApplyLevel)
{
	if (!InAbilitySet.IsValid())
		return FGameplayAbilitySpecHandle();

	FGameplayAbilitySpec AbilitySpec(InAbilitySet.AbilityToGrant);
	AbilitySpec.SourceObject = GetAvatarActor();
	AbilitySpec.Level = ApplyLevel;
	AbilitySpec.GetDynamicSpecSourceTags().AddTag(InAbilitySet.InputTag);

	return GiveAbility(AbilitySpec);
}

/// Sets the level of every granted ability whose Asset Tags contain AbilityTag exactly
bool UGodsentAbilitySystemComponent::SetAbilityLevelByTag(FGameplayTag AbilityTag, int32 NewLevel)
{
	if (!AbilityTag.IsValid())
		return false;

	ABILITYLIST_SCOPE_LOCK();

	bool bFound = false;

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.Ability || !AbilitySpec.Ability->GetAssetTags().HasTagExact(AbilityTag))
			continue;

		AbilitySpec.Level = FMath::Max(NewLevel, 1);
		MarkAbilitySpecDirty(AbilitySpec);
		bFound = true;
	}

	return bFound;
}

/// Gets the level of the first granted ability whose Asset Tags contain AbilityTag exactly, or 0 if it isn't granted
int32 UGodsentAbilitySystemComponent::GetAbilityLevelByTag(FGameplayTag AbilityTag) const
{
	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.Ability && AbilitySpec.Ability->GetAssetTags().HasTagExact(AbilityTag))
			return AbilitySpec.Level;
	}

	return 0;
}

/// Grants a list of hero abilities at a level, binding each to its input tag
void UGodsentAbilitySystemComponent::GrantHeroAbilitySets(const TArray<FGodsentHeroAbilitySet>& InAbilitySets,
	int32 ApplyLevel, TArray<FGameplayAbilitySpecHandle>& OutGrantedAbilitySpecHandles)
{
	for (const FGodsentHeroAbilitySet& AbilitySet : InAbilitySets)
	{
		const FGameplayAbilitySpecHandle GrantedHandle = GrantHeroAbility(AbilitySet, ApplyLevel);

		if (GrantedHandle.IsValid())
			OutGrantedAbilitySpecHandles.AddUnique(GrantedHandle);
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

/// Re-sends every granted hero ability's icon and running cooldown to the HUD
void UGodsentAbilitySystemComponent::BroadcastHeroAbilityUI()
{
	ABILITYLIST_SCOPE_LOCK();

	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (const UGodsentHeroGameplayAbility* HeroAbility = Cast<UGodsentHeroGameplayAbility>(AbilitySpec.Ability))
			HeroAbility->BroadcastAbilityUI(AbilitySpec, AbilityActorInfo.Get());
	}
}
