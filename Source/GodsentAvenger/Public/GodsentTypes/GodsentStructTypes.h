// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "Templates/SubclassOf.h"
#include "GodsentStructTypes.generated.h"

class UGameplayEffect;
class UGodsentHeroGameplayAbility;
class UTexture2D;
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

/// One skill in the skill tree. Tiers chain through Prerequisites (Leap needs Hop, Graced Evasion needs Leap...).
/// Unlocking applies Ability and/or Passive Effect; a node with neither shows as Unavailable (not built yet).
USTRUCT(BlueprintType)
struct FGodsentSkillNode
{
	GENERATED_BODY()

	/// The skill's identity; skill buttons use the same tag
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Skill"))
	FGameplayTag SkillTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;

	/// Skill points needed to unlock
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0"))
	int32 Cost = 1;

	/// Skills that must already be unlocked (normally just the previous tier; two for the stretch-goal combo skills)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Skill"))
	FGameplayTagContainer Prerequisites;

	/// Ability granted when unlocked, or raised to Ability Level if the hero already has it
	/// (Hop = Dodge 1, Leap = Dodge 2...). Fill the Input Tag on every tier; leave it empty for event-triggered passives.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGodsentHeroAbilitySet Ability;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 AbilityLevel = 1;

	/// Infinite effect applied when unlocked (stat boosts, passive tags)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> PassiveEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 PassiveEffectLevel = 1;

	/// False for skills that aren't built yet (no ability and no passive)
	bool IsImplemented() const;
};
