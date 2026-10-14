// Alex Goldberg rights reserved

#include "DataAssets/StartUpData/DataAsset_HeroStartUpData.h"
#include "AbilitySystem/GodsentAbilitySystemComponent.h"

/// Grants the shared startup data, then the hero's input-bound abilities
void UDataAsset_HeroStartUpData::GiveToAbilitySystemComponent(UGodsentAbilitySystemComponent* InASCToGive, int32 ApplyLevel)
{
	Super::GiveToAbilitySystemComponent(InASCToGive, ApplyLevel);

	TArray<FGameplayAbilitySpecHandle> GrantedHandles;
	InASCToGive->GrantHeroAbilitySets(HeroStartUpAbilitySets, ApplyLevel, GrantedHandles);
}
