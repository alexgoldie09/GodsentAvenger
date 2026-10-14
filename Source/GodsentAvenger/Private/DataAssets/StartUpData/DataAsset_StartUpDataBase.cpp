// Alex Goldberg rights reserved

#include "DataAssets/StartUpData/DataAsset_StartUpDataBase.h"
#include "GameplayEffect.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GodsentGameplayAbility.h"

/// Grants the startup abilities and applies the startup effects
void UDataAsset_StartUpDataBase::GiveToAbilitySystemComponent(UGodsentAbilitySystemComponent* InASCToGive, int32 ApplyLevel)
{
	check(InASCToGive);

	GrantAbilities(ActivateOnGivenAbilities, InASCToGive, ApplyLevel);
	GrantAbilities(ReactiveAbilities, InASCToGive, ApplyLevel);

	for (const TSubclassOf<UGameplayEffect>& Effect : StartUpGameplayEffects)
	{
		if (!Effect)
			continue;

		const UGameplayEffect* EffectCDO = Effect->GetDefaultObject<UGameplayEffect>();
		InASCToGive->ApplyGameplayEffectToSelf(EffectCDO, ApplyLevel, InASCToGive->MakeEffectContext());
	}
}

/// Grants a list of abilities at a level
void UDataAsset_StartUpDataBase::GrantAbilities(const TArray<TSubclassOf<UGodsentGameplayAbility>>& AbilitiesToGive,
	UGodsentAbilitySystemComponent* InASCToGive, int32 ApplyLevel)
{
	for (const TSubclassOf<UGodsentGameplayAbility>& Ability : AbilitiesToGive)
	{
		if (!Ability)
			continue;

		FGameplayAbilitySpec AbilitySpec(Ability);
		AbilitySpec.SourceObject = InASCToGive->GetAvatarActor();
		AbilitySpec.Level = ApplyLevel;

		InASCToGive->GiveAbility(AbilitySpec);
	}
}
