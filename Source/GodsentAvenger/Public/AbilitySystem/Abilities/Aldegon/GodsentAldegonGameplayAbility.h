// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GodsentEnemyGameplayAbility.h"
#include "GodsentAldegonGameplayAbility.generated.h"

class UGameplayEffect;
class UGodsentAldegonComponent;

/**
 * Shared base for Aldegon's C++ moves (Soul Beam, Death Wave).
 * - The cooldown starts when the move ends (so the curve table number is the gap between moves).
 * - Use Attack Speed: the montages play at his Attack Speed, so frenzy speeds them up.
 */
UCLASS(Abstract)
class GODSENTAVENGER_API UGodsentAldegonGameplayAbility : public UGodsentEnemyGameplayAbility
{
	GENERATED_BODY()

public:
	UGodsentAldegonGameplayAbility();

protected:
	/// Montages play at his Attack Speed (frenzy makes this faster)
	UPROPERTY(EditDefaultsOnly, Category = "Aldegon")
	bool bUseAttackSpeed = true;

	/// The damage effect (GE_Shared_DealDamage, same as the zombie's attacks)
	UPROPERTY(EditDefaultsOnly, Category = "Aldegon|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/// Play rate for this move's montages
	float GetMoveMontageRate() const;

	UGodsentAldegonComponent* GetAldegonComponent() const;

	/// Who he's fighting (his blackboard target, or the arena's challenger)
	AActor* GetCombatTarget() const;

	/// Builds the damage for one hit. Magic skips the target's defence.
	FGameplayEffectSpecHandle MakeMoveDamageSpec(const FScalableFloat& Damage, bool bMagic, bool bCausesHitStun);

	/// Ends the move, starting the cooldown first if asked (not when it was cancelled)
	void FinishMove(bool bStartCooldown);
};
