// Alex Goldberg rights reserved

#include "GodsentTypes/GodsentStructTypes.h"
#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility.h"

/// True when both the input tag and the ability are set
bool FGodsentHeroAbilitySet::IsValid() const
{
	return InputTag.IsValid() && AbilityToGrant;
}
