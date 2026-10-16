// Alex Goldberg rights reserved

#include "AbilitySystem/Abilities/GodsentEnemyGameplayAbility.h"
#include "GodsentGameplayTags.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "Characters/GodsentEnemyCharacter.h"

/// Gets (and caches) the enemy character that owns this ability
AGodsentEnemyCharacter* UGodsentEnemyGameplayAbility::GetEnemyCharacterFromActorInfo()
{
	if (!CachedGodsentEnemyCharacter.IsValid())
		CachedGodsentEnemyCharacter = Cast<AGodsentEnemyCharacter>(GetAvatarActorFromActorInfo());

	return CachedGodsentEnemyCharacter.IsValid() ? CachedGodsentEnemyCharacter.Get() : nullptr;
}

/// Gets the enemy's combat component
UEnemyCombatComponent* UGodsentEnemyGameplayAbility::GetEnemyCombatComponentFromActorInfo()
{
	AGodsentEnemyCharacter* EnemyCharacter = GetEnemyCharacterFromActorInfo();
	return EnemyCharacter ? EnemyCharacter->GetEnemyCombatComponent() : nullptr;
}

/// Builds the damage spec for an enemy attack from a per-level damage value
FGameplayEffectSpecHandle UGodsentEnemyGameplayAbility::CreateEnemyDamageEffectSpecHandle(
	TSubclassOf<UGameplayEffect> EffectClass, const FScalableFloat& InDamageScalableFloat)
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

	EffectSpecHandle.Data->SetSetByCallerMagnitude(
		GodsentGameplayTags::Shared_SetByCaller_BaseDamage,
		InDamageScalableFloat.GetValueAtLevel(GetAbilityLevel()));

	return EffectSpecHandle;
}
