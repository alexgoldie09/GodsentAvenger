// Alex Goldberg rights reserved

#include "GodsentTypes/GodsentStructTypes.h"
#include "GameplayEffect.h"
#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility.h"

/// True when both the input tag and the ability are set
bool FGodsentHeroAbilitySet::IsValid() const
{
	return InputTag.IsValid() && AbilityToGrant;
}

/// False for skills that aren't built yet (no ability and no passive)
bool FGodsentSkillNode::IsImplemented() const
{
	return Ability.AbilityToGrant || PassiveEffect;
}
