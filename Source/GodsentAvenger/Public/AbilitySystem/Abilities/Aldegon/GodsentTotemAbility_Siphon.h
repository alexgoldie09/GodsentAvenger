// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "AbilitySystem/Abilities/GodsentEnemyGameplayAbility.h"
#include "GodsentTotemAbility_Siphon.generated.h"

class UGameplayEffect;

/**
 * The bone totem's Life Siphon (GA_BoneTotem_Siphon): always on from the moment it's summoned, like the ghost's Wail.
 * Every Siphon Interval it hurts the player inside Siphon Radius and, if it hit, heals Aldegon (its Instigator)
 * by Heal Percent of his max health. Each of those ticks shakes the totem (Godsent Shake) and plays the Pulse Cue.
 * Stops when the totem dies (its death cancels Enemy.Ability).
 */
UCLASS()
class GODSENTAVENGER_API UGodsentTotemAbility_Siphon : public UGodsentEnemyGameplayAbility
{
	GENERATED_BODY()

public:
	UGodsentTotemAbility_Siphon();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/// GE_Aldegon_RangedDamage (no Melee tag, so Holy Retribution doesn't answer it)
	UPROPERTY(EditDefaultsOnly, Category = "Siphon")
	TSubclassOf<UGameplayEffect> DamageEffect;

	// ---- Stats (CT_TotemStats rows) ----

	/// Damage per tick to each player in range
	UPROPERTY(EditDefaultsOnly, Category = "Siphon|Stats")
	FScalableFloat SiphonDamage = 1.f;

	/// Seconds between ticks
	UPROPERTY(EditDefaultsOnly, Category = "Siphon|Stats")
	FScalableFloat SiphonInterval = 1.f;

	/// How close the player has to be (cm)
	UPROPERTY(EditDefaultsOnly, Category = "Siphon|Stats")
	FScalableFloat SiphonRadius = 1.f;

	/// Each tick that hits heals Aldegon this % of his max health
	UPROPERTY(EditDefaultsOnly, Category = "Siphon|Stats")
	FScalableFloat HealPercent = 1.f;

	// ---- Rules ----

	/// Skips the player's defence
	UPROPERTY(EditDefaultsOnly, Category = "Siphon|Rules")
	bool bMagicDamage = true;

	/// A tick staggers the player
	UPROPERTY(EditDefaultsOnly, Category = "Siphon|Rules")
	bool bCausesHitStun = false;

	/// On: shakes and plays the cue every tick even with nobody in range. Off: only when it's draining someone.
	UPROPERTY(EditDefaultsOnly, Category = "Siphon|Rules")
	bool bPulseWithoutTarget = false;

	// ---- Visuals ----

	/// How hard the totem shakes each tick (1 = its Godsent Shake settings)
	UPROPERTY(EditDefaultsOnly, Category = "Siphon|Visuals", meta = (ClampMin = "0.0"))
	float ShakeStrength = 1.f;

	/// GameplayCue each tick: Location = the totem, Effect Causer = the player it hit (if any), Instigator = Aldegon. Empty = none.
	UPROPERTY(EditDefaultsOnly, Category = "Siphon|Visuals", meta = (Categories = "GameplayCue"))
	FGameplayTag PulseCueTag;

	/// Draws the siphon range each tick (green = hit someone, grey = nobody)
	UPROPERTY(EditDefaultsOnly, Category = "Siphon|Debug")
	bool bDrawDebug = false;

private:
	/// Waits one Siphon Interval, then ticks
	void WaitForNextTick();

	UFUNCTION()
	void OnTickDelayFinished();

	/// Hurts whoever's in range; returns the first one hit (null if nobody)
	AActor* DrainTargets();

	/// Who summoned the totem (Aldegon), or null for a totem placed by hand
	AActor* GetSummoner() const;

	/// Heals the summoner by Heal Percent of his max health
	void HealSummoner();

	void PlayPulse(AActor* HitTarget);
};
