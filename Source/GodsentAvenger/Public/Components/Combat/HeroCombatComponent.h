// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "HeroCombatComponent.generated.h"

class AGodsentHeroWeapon;

/**
 * The hero's combat component: weapon lookups and melee hit events.
 */
UCLASS()
class GODSENTAVENGER_API UHeroCombatComponent : public UPawnCombatComponent
{
	GENERATED_BODY()

public:
	/// Gets a carried hero weapon by tag
	UFUNCTION(BlueprintCallable, Category = "Godsent|Combat")
	AGodsentHeroWeapon* GetHeroCarriedWeaponByTag(FGameplayTag InWeaponTag) const;

	/// Gets the equipped hero weapon, or nullptr
	UFUNCTION(BlueprintCallable, Category = "Godsent|Combat")
	AGodsentHeroWeapon* GetHeroCurrentEquippedWeapon() const;

	/// Gets the equipped weapon's base damage at a level (0 with no weapon equipped)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Combat")
	float GetHeroCurrentEquippedWeaponDamageAtLevel(float InLevel) const;

	/// Sends MeleeHit (for the attack ability to apply damage) and HitPause
	virtual void OnHitTargetActor(AActor* HitActor) override;

	/// Sends HitPause as the blade leaves the target
	virtual void OnWeaponPulledFromTargetActor(AActor* InteractedActor) override;

	/// Remembers the enemy that was just parried (set by the block ability at Hallowed Riposte),
	/// so the riposte can turn to face it
	UFUNCTION(BlueprintCallable, Category = "Godsent|Combat")
	void SetRiposteTarget(AActor* InRiposteTarget) { RiposteTarget = InRiposteTarget; }

	/// The enemy that was last parried, or nullptr if it's gone
	UFUNCTION(BlueprintPure, Category = "Godsent|Combat")
	AActor* GetRiposteTarget() const { return RiposteTarget.Get(); }

private:
	/// Weak so a parried enemy that dies or despawns doesn't linger here
	TWeakObjectPtr<AActor> RiposteTarget;
};
