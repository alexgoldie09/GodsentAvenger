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
 * - the beam runs out to Range, stopping at walls;
 * - everyone hostile inside it (Radius) takes Damage Per Tick every Tick Interval. Unblockable; dodge i-frames avoid it.
 * Interrupting the montage stops the beam straight away.
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

	/// How thick the beam is for hitting (cm)
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Stats")
	FScalableFloat Radius = 1.f;

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

	/// Placeholder (or real) beam effect, attached to the socket while firing
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Look")
	TObjectPtr<UNiagaraSystem> BeamEffect;

	/// Niagara vector parameters set every frame (world space)
	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Look")
	FName BeamStartParameter = TEXT("BeamStart");

	UPROPERTY(EditDefaultsOnly, Category = "Soul Beam|Look")
	FName BeamEndParameter = TEXT("BeamEnd");

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

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> BeamComponent;

	bool bFiring = false;
	float TimeSinceDamageTick = 0.f;
	bool bWarnedMissingSocket = false;
};
