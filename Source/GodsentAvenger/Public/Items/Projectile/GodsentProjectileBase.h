// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "GodsentProjectileBase.generated.h"

class UNiagaraComponent;
class UProjectileMovementComponent;
class USphereComponent;

/**
 * Enemy projectile, ported from the tutorial with changes:
 * - optional homing: flies along its launch direction for Homing Delay, then curves towards the target with limited
 *   acceleration. It stops homing once close or past the target, so a late dodge makes it miss;
 * - hostile pawns are overlapped, not blocked: a rolling (invincible) player is passed straight through;
 * - uses the same block rules as melee: parry or full block = no damage, partial block = damage still lands
 *   (pure damage ignores the block's defence bonus anyway), hit outside the guard while blocking = guard break;
 * - never sends a hit react itself: damage reactions come from the damage effect (Shared.Effect.HitStun);
 * - walls and floors stop it; it fizzles if it times out or its caster is gone.
 *
 * Spawn it with Spawn Projectile Volley (enemy abilities) or Spawn Actor with the exposed pins.
 */
UCLASS()
class GODSENTAVENGER_API AGodsentProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AGodsentProjectileBase();

	/// Sets the damage and the homing target. Call before the projectile finishes spawning (the volley helper does this).
	UFUNCTION(BlueprintCallable, Category = "Godsent|Projectile")
	void InitProjectile(const FGameplayEffectSpecHandle& InDamageEffectSpecHandle, AActor* InHomingTarget);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void LifeSpanExpired() override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> ProjectileCollisionSphere;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UNiagaraComponent> ProjectileNiagaraComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	/// Damage applied to the pawn it hits (built by the casting ability)
	UPROPERTY(BlueprintReadOnly, Category = "Projectile", meta = (ExposeOnSpawn = "true"))
	FGameplayEffectSpecHandle ProjectileDamageEffectSpecHandle;

	/// What it curves towards after Homing Delay. None = flies straight.
	UPROPERTY(BlueprintReadOnly, Category = "Projectile", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<AActor> HomingTarget;

	/// Off = ignores blocking entirely (still dodgeable)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Block")
	bool bCanBeBlocked = true;

	/// How directly the player must face the projectile for a block to count (same meaning as the enemy combat component's)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Block", meta = (EditCondition = "bCanBeBlocked"))
	float BlockFacingThreshold = -0.1f;

	/// Seconds of flying along the launch direction (the upward rise) before homing starts
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Homing", meta = (ClampMin = "0.0", Units = "s"))
	float HomingDelay = 0.45f;

	/// How hard it turns towards the target. Lower = wider curves and easier to dodge.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Homing", meta = (ClampMin = "0.0"))
	float HomingAcceleration = 1600.f;

	/// Stops homing once this close, so it carries straight on and a last-moment dodge makes it miss
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Homing", meta = (ClampMin = "0.0", Units = "cm"))
	float HomingCommitDistance = 250.f;

	/// Stops homing after this long, whatever happens (it can't orbit the player forever)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Homing", meta = (ClampMin = "0.0", Units = "s"))
	float MaxHomingDuration = 3.5f;

	/// Impact effects (Niagara, sound). Called once, just before the projectile is destroyed.
	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile", meta = (DisplayName = "On Projectile Impact"))
	void BP_OnProjectileImpact(const FVector& ImpactLocation, EGodsentProjectileImpact ImpactType);

private:
	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/// Block, damage and guard break against a hostile pawn. Returns false if the projectile should keep flying.
	bool HandlePawnHit(APawn* HitPawn);

	void StartHoming();
	void StopHoming();

	/// Plays the impact event once and destroys the projectile
	void FinishProjectile(const FVector& ImpactLocation, EGodsentProjectileImpact ImpactType);

	FTimerHandle HomingStartTimerHandle;
	float HomingElapsedTime = 0.f;
	bool bIsHoming = false;
	bool bHasFinished = false;
};
