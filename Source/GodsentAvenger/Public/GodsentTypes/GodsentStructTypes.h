// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "Templates/SubclassOf.h"
#include "GodsentStructTypes.generated.h"

class UGodsentHeroGameplayAbility;
class UGodsentHeroLinkedAnimLayer;
class UInputMappingContext;

/// An ability granted to the hero together with the input tag that activates it.
/// Used by the hero startup data, the weapon, and later the skill tree.
USTRUCT(BlueprintType)
struct FGodsentHeroAbilitySet
{
	GENERATED_BODY()

	/// The input tag that activates the ability
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	/// The ability granted to the hero
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGodsentHeroGameplayAbility> AbilityToGrant;

	/// True when both the input tag and the ability are set
	bool IsValid() const;
};

/// Everything the equipped weapon brings with it.
/// The tutorial's special weapon abilities and icon fields are gone (one sword, no weapon specials).
USTRUCT(BlueprintType)
struct FGodsentHeroWeaponData
{
	GENERATED_BODY()

	/// Linked anim layer for this weapon's locomotion
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGodsentHeroLinkedAnimLayer> WeaponAnimLayerToLink;

	/// Mapping context added while this weapon is equipped (light attack input)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UInputMappingContext> WeaponInputMappingContext;

	/// Abilities granted while this weapon is equipped (the light attack)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "InputTag"))
	TArray<FGodsentHeroAbilitySet> DefaultWeaponAbilities;

	/// Base damage per level, read by the light attack and scaled by the damage calculation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FScalableFloat WeaponBaseDamage;
};
