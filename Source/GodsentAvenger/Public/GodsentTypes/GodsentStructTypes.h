// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/SoftObjectPtr.h"
#include "GodsentStructTypes.generated.h"

class AActor;
class AGodsentEnemyCharacter;
class USoundBase;
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

/// One possible enemy in a weighted pool (enemy spawners and Aldegon's Summon / Life Siphon).
/// A soft reference: the enemy Blueprint only loads when something is about to spawn it.
USTRUCT(BlueprintType)
struct FGodsentEnemySpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TSoftClassPtr<AGodsentEnemyCharacter> EnemyClass;

	/// How likely this enemy is compared with the others (Zombie 3 + Ghost 1 = 75% zombies). 0 = never.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn", meta = (ClampMin = "0.0"))
	float Weight = 1.f;

	/// The class, loading it now (with a warning) if nobody loaded it ahead of time
	TSubclassOf<AGodsentEnemyCharacter> ResolveClass() const;

	/// Weighted random pick. Entries with no class or 0 weight are skipped. Null if nothing can be picked.
	static const FGodsentEnemySpawnEntry* PickWeighted(const TArray<FGodsentEnemySpawnEntry>& Pool);

	/// Every class in the pool, to load them all in one go
	static TArray<FSoftObjectPath> GetClassPaths(const TArray<FGodsentEnemySpawnEntry>& Pool);

	/// True once every class in the pool is in memory
	static bool IsPoolLoaded(const TArray<FGodsentEnemySpawnEntry>& Pool);
};

/// Raises one of Aldegon's summon caps once his health is at or below a %
USTRUCT(BlueprintType)
struct FGodsentAldegonCapStep
{
	GENERATED_BODY()

	FGodsentAldegonCapStep() = default;

	FGodsentAldegonCapStep(float InAtHealthPercent, int32 InMaxAlive)
		: AtHealthPercent(InAtHealthPercent), MaxAlive(InMaxAlive)
	{
	}

	/// At or below this health % ...
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cap", meta = (ClampMin = "0.0", ClampMax = "100.0", Units = "Percent"))
	float AtHealthPercent = 50.f;

	/// ... this many can be alive at once
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cap", meta = (ClampMin = "0"))
	int32 MaxAlive = 1;
};

/// One line of dialogue for the Dialogue subsystem: who says it, what they say, and how long to wait first
USTRUCT(BlueprintType)
struct FGodsentDialogueLine
{
	GENERATED_BODY()

	FGodsentDialogueLine() = default;

	FGodsentDialogueLine(AActor* InSpeaker, USoundBase* InSound, const FGameplayTag& InHeroVoiceLine, float InDelayBefore)
		: Speaker(InSpeaker), Sound(InSound), HeroVoiceLine(InHeroVoiceLine), DelayBefore(InDelayBefore)
	{
	}

	/// Who says it (the sound plays from them)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TObjectPtr<AActor> Speaker;

	/// The sound to play. Leave empty to use Hero Voice Line instead.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TObjectPtr<USoundBase> Sound;

	/// Only used when Sound is empty: this line from the speaker's voice (their Heir Appearance voice set)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (Categories = "Voice"))
	FGameplayTag HeroVoiceLine;

	/// Wait this long before the line starts (after the last line has finished)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (ClampMin = "0.0", Units = "s"))
	float DelayBefore = 0.f;
};
