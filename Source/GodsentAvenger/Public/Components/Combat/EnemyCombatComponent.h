// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "EnemyCombatComponent.generated.h"

/**
 * Enemy combat component: resolves the hero's block and parry, then sends MeleeHit for unblocked damage.
 */
UCLASS()
class GODSENTAVENGER_API UEnemyCombatComponent : public UPawnCombatComponent
{
	GENERATED_BODY()

public:
	/// Resolves block and parry; sends MeleeHit unless the hit was fully blocked or parried
	virtual void OnHitTargetActor(AActor* HitActor) override;

protected:
	/// Turns the left or right hand collision box on or off
	virtual void ToggleBodyCollisionBoxCollision(bool bShouldEnable, EToggleDamageType ToggleDamageType) override;

	/// How directly the hero must face the attacker for a block to count (dot product of forward vectors)
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float BlockFacingThreshold = -0.1f;
};
