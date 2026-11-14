// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GodsentHeroGameplayAbility.h"
#include "ScalableFloat.h"
#include "GodsentHeroGameplayAbility_Revive.generated.h"

class UAbilitySystemComponent;
class UAnimMontage;

/**
 * Saintly Revival (GA_Hero_Revive). Triggered by Player.Event.Revive, which the attribute set sends instead of
 * killing the heir the first time their health hits 0 (once per heir, saved in the lineage).
 *
 *  1. The hero is Invincible and plays Revive Montage (the kneel and rise from the cast animation).
 *  2. At Player.Event.Revive.Rise (an anim notify), or at the end of the montage if there's no notify:
 *     health refills to max, nearby enemies are knocked back like the heal release, and the cue and voice play.
 *  3. The montage ends and control comes back. Invincible stays on for Grace Time more.
 *
 * Grant it from the Saintly Revival skill node (Ability, no Input Tag) alongside its passive effect
 * (Player.Passive.SaintlyRevival), which the attribute set checks.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentHeroGameplayAbility_Revive : public UGodsentHeroGameplayAbility
{
	GENERATED_BODY()

public:
	UGodsentHeroGameplayAbility_Revive();

protected:
	//~ Begin UGameplayAbility Interface.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End UGameplayAbility Interface.

	/// Kneel down and get back up (made from the cast animation)
	UPROPERTY(EditDefaultsOnly, Category = "Revive")
	TObjectPtr<UAnimMontage> ReviveMontage;

	/// Section to start from (None = the start of the montage)
	UPROPERTY(EditDefaultsOnly, Category = "Revive")
	FName MontageStartSection = NAME_None;

	/// Seconds the hero stays Invincible after getting back up (curve table: Hero.ReviveGraceTime)
	UPROPERTY(EditDefaultsOnly, Category = "Revive")
	FScalableFloat GraceTime;

	/// Knockback when the hero rises, same rows as the heal release
	UPROPERTY(EditDefaultsOnly, Category = "Revive|Knockback")
	FScalableFloat KnockbackRadius;

	UPROPERTY(EditDefaultsOnly, Category = "Revive|Knockback")
	FScalableFloat KnockbackStrength;

	UPROPERTY(EditDefaultsOnly, Category = "Revive|Knockback")
	FScalableFloat KnockbackUpForce;

	/// Played on the hero as they rise (e.g. a burst of light). Optional.
	UPROPERTY(EditDefaultsOnly, Category = "Revive|Feedback", meta = (Categories = "GameplayCue"))
	FGameplayTag RiseCueTag;

	/// Voice line as they rise. Optional.
	UPROPERTY(EditDefaultsOnly, Category = "Revive|Feedback", meta = (Categories = "Voice"))
	FGameplayTag RiseVoiceLine;

private:
	UFUNCTION()
	void OnRiseEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageFinished();

	/// Refills health, knocks enemies back and plays the feedback (only once)
	void Rise();

	/// Takes off the Invincible tag this ability added
	void RemoveInvincible();

	bool bHasRisen = false;
	bool bAddedInvincible = false;

	/// Who got the Invincible tag (the grace timer runs after the ability has ended)
	TWeakObjectPtr<UAbilitySystemComponent> InvincibleASC;

	FTimerHandle GraceTimerHandle;
};
