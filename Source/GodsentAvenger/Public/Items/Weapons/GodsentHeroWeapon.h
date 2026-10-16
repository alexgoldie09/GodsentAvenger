// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "Items/Weapons/GodsentWeaponBase.h"
#include "GodsentTypes/GodsentStructTypes.h"
#include "GodsentHeroWeapon.generated.h"

/**
 * The hero's weapon (one sword for the whole game). Remembers the abilities it granted so they can be removed on unequip.
 */
UCLASS()
class GODSENTAVENGER_API AGodsentHeroWeapon : public AGodsentWeaponBase
{
	GENERATED_BODY()

public:
	/// Anim layer, mapping context, abilities and base damage for this weapon
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponData")
	FGodsentHeroWeaponData HeroWeaponData;

	/// Stores the handles of the abilities granted on equip
	UFUNCTION(BlueprintCallable)
	void AssignGrantedAbilitySpecHandles(const TArray<FGameplayAbilitySpecHandle>& InGrantedAbilitySpecHandles);

	/// The handles of the abilities granted on equip
	UFUNCTION(BlueprintPure)
	TArray<FGameplayAbilitySpecHandle> GetGrantedAbilitySpecHandles() const;

private:
	TArray<FGameplayAbilitySpecHandle> GrantedAbilitySpecHandles;
};
