// Alex Goldberg rights reserved

#include "Items/Weapons/GodsentHeroWeapon.h"

/// Stores the handles of the abilities granted on equip
void AGodsentHeroWeapon::AssignGrantedAbilitySpecHandles(const TArray<FGameplayAbilitySpecHandle>& InGrantedAbilitySpecHandles)
{
	GrantedAbilitySpecHandles = InGrantedAbilitySpecHandles;
}

/// The handles of the abilities granted on equip
TArray<FGameplayAbilitySpecHandle> AGodsentHeroWeapon::GetGrantedAbilitySpecHandles() const
{
	return GrantedAbilitySpecHandles;
}
