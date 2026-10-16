// Alex Goldberg rights reserved

#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GodsentAttributeSet.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "Characters/GodsentHeroCharacter.h"
#include "Controllers/GodsentHeroController.h"
#include "Components/Combat/HeroCombatComponent.h"
#include "GodsentGameplayTags.h"

/// Gets (and caches) the hero character that owns this ability
AGodsentHeroCharacter* UGodsentHeroGameplayAbility::GetHeroCharacterFromActorInfo()
{
	if (!CachedHeroCharacter.IsValid())
		CachedHeroCharacter = Cast<AGodsentHeroCharacter>(GetAvatarActorFromActorInfo());

	return CachedHeroCharacter.IsValid() ? CachedHeroCharacter.Get() : nullptr;
}

/// Gets (and caches) the hero's player controller
AGodsentHeroController* UGodsentHeroGameplayAbility::GetHeroControllerFromActorInfo()
{
	if (!CachedHeroController.IsValid() && CurrentActorInfo)
		CachedHeroController = Cast<AGodsentHeroController>(CurrentActorInfo->PlayerController.Get());

	return CachedHeroController.IsValid() ? CachedHeroController.Get() : nullptr;
}

/// Gets the hero's combat component
UHeroCombatComponent* UGodsentHeroGameplayAbility::GetHeroCombatComponentFromActorInfo()
{
	AGodsentHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo();
	return HeroCharacter ? HeroCharacter->GetHeroCombatComponent() : nullptr;
}

/// Builds the damage spec for a hero attack
FGameplayEffectSpecHandle UGodsentHeroGameplayAbility::CreateHeroDamageEffectSpecHandle(
	TSubclassOf<UGameplayEffect> EffectClass, float InWeaponBaseDamage, FGameplayTag InCurrentAttackTypeTag,
	int32 InUsedComboCount)
{
	check(EffectClass);

	FGameplayEffectContextHandle ContextHandle = GetGodsentAbilitySystemComponentFromActorInfo()->MakeEffectContext();
	ContextHandle.SetAbility(this);
	ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());
	ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());

	FGameplayEffectSpecHandle EffectSpecHandle = GetGodsentAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
		EffectClass,
		GetAbilityLevel(),
		ContextHandle);

	EffectSpecHandle.Data->SetSetByCallerMagnitude(GodsentGameplayTags::Shared_SetByCaller_BaseDamage, InWeaponBaseDamage);

	if (InCurrentAttackTypeTag.IsValid())
		EffectSpecHandle.Data->SetSetByCallerMagnitude(InCurrentAttackTypeTag, InUsedComboCount);

	return EffectSpecHandle;
}

/// Gets the total and remaining time of the cooldown effect carrying InCooldownTag
bool UGodsentHeroGameplayAbility::GetAbilityRemainingCooldownByTag(FGameplayTag InCooldownTag, float& TotalCooldownTime,
	float& RemainingCooldownTime)
{
	check(InCooldownTag.IsValid());

	// Start from zero so "no cooldown running" returns false instead of reading uninitialised values
	TotalCooldownTime = 0.f;
	RemainingCooldownTime = 0.f;

	const FGameplayEffectQuery CooldownQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(InCooldownTag.GetSingleTagContainer());
	const TArray<TPair<float, float>> TimeRemainingAndDuration = GetAbilitySystemComponentFromActorInfo()->GetActiveEffectsTimeRemainingAndDuration(CooldownQuery);

	if (!TimeRemainingAndDuration.IsEmpty())
	{
		RemainingCooldownTime = TimeRemainingAndDuration[0].Key;
		TotalCooldownTime = TimeRemainingAndDuration[0].Value;
	}

	return RemainingCooldownTime > 0.f;
}

/// The hero's AttackSpeed attribute, for the Rate pin of Play Montage And Wait
float UGodsentHeroGameplayAbility::GetAttackSpeedMontageRate() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!ASC)
		return 1.f;

	bool bFound = false;
	const float AttackSpeed = ASC->GetGameplayAttributeValue(UGodsentAttributeSet::GetAttackSpeedAttribute(), bFound);

	return bFound ? FMath::Max(AttackSpeed, 0.1f) : 1.f;
}
