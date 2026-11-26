// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/Abilities/Aldegon/GodsentAldegonGameplayAbility.h"
#include "GodsentAldegonAbility_SoulBeam.generated.h"

class UAnimMontage;
class UNiagaraComponent;
class UNiagaraSystem;

/**
 * Soul Beam (GA_Aldegon_SoulBeam): plays the Beam montage. Between the Beam Start and Beam End events
 * (AN_SendGameplayEventToOwner notifies on the montage) the beam fires from Beam Socket:
 * - he turns towards the player at Turn Rate (slow, so it can be outrun);
 * - the beam shoots out to Range over Extend Time (it only hurts as far as it has reached), stopping at walls;
 * - everyone hostile inside it (Radius) takes Damage Per Tick every Tick Interval. Unblockable; dodge i-frames avoid it.
 * Interrupting the montage stops the beam straight away (the effect winds down rather than popping).
 *
 * The effect (Beam Effect) is told every frame where the beam is, so it matches the hits exactly:
 * - Beam Start / Beam End (Position, world): for a Niagara beam or ribbon;
 * - Beam Length / Beam Radius (float), Beam Extend (0-1 while shooting out), Beam Blocked (bool: stopped by a wall);
 * - the component itself sits at the beam start, its X axis pointing down the beam (for a mesh along local X).
 */
UCLASS()
class GODSENTAVENGER_API UGodsentAldegonAbility_SoulBeam : public UGodsentAldegonGameplayAbility
{
	GENERATED_BODY()

public:
	UGodsentAldegonAbility_SoulBeam();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam")
	TObjectPtr<UAnimMontage> BeamMontage;

	/// Notify event that starts the beam
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam", meta = (Categories = "Enemy.Event"))
	FGameplayTag BeamStartEventTag;

	/// Notify event that stops it
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam", meta = (Categories = "Enemy.Event"))
	FGameplayTag BeamEndEventTag;

	/// Where the beam comes out (a socket just in front of his left hand)
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam")
	FName BeamSocketName = TEXT("BeamSocket");

	/// Used if Beam Socket isn't on the skeleton
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam")
	FName FallbackBoneName = TEXT("LHand");

	// ---- Stats (CT_AldegonStats rows) ----

	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Stats")
	FScalableFloat DamagePerTick = 1.f;

	/// Seconds between damage ticks
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Stats")
	FScalableFloat TickInterval = 1.f;

	/// How fast he turns to follow the player while firing (degrees/s)
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Stats")
	FScalableFloat TurnRate = 1.f;

	/// How far the beam reaches (cm)
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Stats")
	FScalableFloat Range = 1.f;

	/// How thick the beam is for hitting (cm). The effect gets the same number (Beam Radius).
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Stats")
	FScalableFloat Radius = 1.f;

	/// Seconds to shoot out from his hand to full Range. It only hurts as far as it has reached. 0 = instantly.
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Stats")
	FScalableFloat ExtendTime = 1.f;

	// ---- Rules ----

	/// Skips the target's defence (pure magic, like Mindfire)
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Rules")
	bool bMagicDamage = true;

	/// Each tick staggers the target (off: it would stun-lock)
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Rules")
	bool bCausesHitStun = false;

	/// What stops the beam
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Rules")
	TEnumAsByte<ECollisionChannel> WallObjectType = ECC_WorldStatic;

	// ---- Look ----

	/// The beam effect, spawned while firing
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Look")
	TObjectPtr<UNiagaraSystem> BeamEffect;

	/// Niagara Position parameters set every frame (world space)
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Look")
	FName BeamStartParameter = TEXT("BeamStart");

	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Look")
	FName BeamEndParameter = TEXT("BeamEnd");

	/// Niagara float: start to end this frame (cm)
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Look")
	FName BeamLengthParameter = TEXT("BeamLength");

	/// Niagara float: the hit radius (cm), so the visual is exactly as thick as what hurts
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Look")
	FName BeamRadiusParameter = TEXT("BeamRadius");

	/// Niagara float: 0 to 1 while it shoots out, then 1
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Look")
	FName BeamExtendParameter = TEXT("BeamExtend");

	/// Niagara bool: true while a wall is stopping it (for impact sparks)
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Look")
	FName BeamBlockedParameter = TEXT("BeamBlocked");

	/// When the beam stops, the effect stops spawning and its particles finish; it's removed after this long at most (s)
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Look", meta = (ClampMin = "0.0", Units = "s"))
	float WindDownTime = 0.5f;

	/// GameplayCue on the player for each damage tick (Location = where the beam touches them, Normal = back along the beam). Empty = none.
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Look", meta = (Categories = "GameplayCue"))
	FGameplayTag HitCueTag;

	/// Draws the beam's hit shape
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Debug")
	bool bDrawDebug = false;

private:
	UFUNCTION()
	void OnBeamStartEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnBeamEndEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageFinished();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnTick(float DeltaTime);

	void StartBeam();
	void StopBeam();

	/// Where the beam starts this frame
	FVector GetBeamStart() const;

	/// Passes this frame's beam to the effect
	void UpdateBeamEffect(const FVector& BeamStart, const FVector& BeamEnd, const FVector& BeamDirection, float BeamRadius, float ExtendAlpha, bool bBlocked);

	/// The player was hit this tick
	void PlayHitCue(APawn* HitPawn, const FVector& HitLocation, const FVector& BeamDirection) const;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> BeamComponent;

	bool bFiring = false;

	/// Seconds since the beam started (for Extend Time)
	float FiringTime = 0.f;
	float TimeSinceDamageTick = 0.f;
	bool bWarnedMissingSocket = false;
};
