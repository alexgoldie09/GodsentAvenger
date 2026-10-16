// Alex Goldberg rights reserved

#include "DataAssets/StartUpData/DataAsset_EnemyStartUpData.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GodsentEnemyGameplayAbility.h"

/// Grants the shared startup data, then the enemy's combat abilities
void UDataAsset_EnemyStartUpData::GiveToAbilitySystemComponent(UGodsentAbilitySystemComponent* InASCToGive, int32 ApplyLevel)
{
	Super::GiveToAbilitySystemComponent(InASCToGive, ApplyLevel);

	for (const TSubclassOf<UGodsentEnemyGameplayAbility>& AbilityClass : EnemyCombatAbilities)
	{
		if (!AbilityClass)
			continue;

		FGameplayAbilitySpec AbilitySpec(AbilityClass);
		AbilitySpec.SourceObject = InASCToGive->GetAvatarActor();
		AbilitySpec.Level = ApplyLevel;

		InASCToGive->GiveAbility(AbilitySpec);
	}
}
