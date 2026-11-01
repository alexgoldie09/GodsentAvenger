// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "ScalableFloat.h"
#include "AbilitySystem/Abilities/GodsentGameplayAbility.h"
#include "GodsentEnemyGameplayAbility.generated.h"

class AGodsentEnemyCharacter;
class AGodsentProjectileBase;
class UEnemyCombatComponent;

/**
 * Base class for enemy abilities (attacks, hit react, death).
 */
UCLASS()
class GODSENTAVENGER_API UGodsentEnemyGameplayAbility : public UGodsentGameplayAbility
{
	GENERATED_BODY()

public:
	/// Gets (and caches) the enemy character that owns this ability
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	AGodsentEnemyCharacter* GetEnemyCharacterFromActorInfo();

	/// Gets the enemy's combat component
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	UEnemyCombatComponent* GetEnemyCombatComponentFromActorInfo();

	/// Builds the damage spec for an enemy attack from a per-level damage value
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	FGameplayEffectSpecHandle CreateEnemyDamageEffectSpecHandle(TSubclassOf<UGameplayEffect> EffectClass, const FScalableFloat& InDamageScalableFloat);

	/// The enemy's AttackSpeed attribute, for the Rate pin of Play Montage And Wait (1 = normal, 0.8 = 20% slower).
	/// Set per enemy type through its start-up stats effect (e.g. a slow zombie).
	UFUNCTION(BlueprintPure, Category = "Godsent|Ability")
	float GetAttackSpeedMontageRate() const;

	/// Spawns a fan of projectiles from Spawn Location, all sharing one damage spec.
	/// They launch angled up by Launch Pitch and spread across Spread Angle (centred on the direction to the target),
	/// then home in on Homing Target after the projectile's Homing Delay. Returns how many were spawned.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Ability")
	int32 SpawnProjectileVolley(TSubclassOf<AGodsentProjectileBase> ProjectileClass, const FGameplayEffectSpecHandle& DamageSpecHandle,
		AActor* HomingTarget, FVector SpawnLocation, int32 ProjectileCount = 3, float SpreadAngle = 70.f, float LaunchPitch = 55.f);

private:
	TWeakObjectPtr<AGodsentEnemyCharacter> CachedGodsentEnemyCharacter;
};
