// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/Abilities/Aldegon/GodsentAldegonGameplayAbility.h"
#include "GodsentAldegonAbility_DeathWave.generated.h"

class AGodsentDeathWaveRing;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

/**
 * Death Wave (GA_Aldegon_DeathWave): drops expanding rings along the ground.
 * How many in a row comes from his health (Godsent Aldegon: Death Wave Thresholds, capped by Max Death Waves).
 * The first wave plays First Wave Montage (Burst), every extra one Next Wave Montage (BurstAgain),
 * with Wave Spacing seconds between them. Each montage drops a ring on its Wave Event notify.
 * The cooldown starts after the whole combo.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentAldegonAbility_DeathWave : public UGodsentAldegonGameplayAbility
{
	GENERATED_BODY()

public:
	UGodsentAldegonAbility_DeathWave();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/// The first wave (Burst)
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave")
	TObjectPtr<UAnimMontage> FirstWaveMontage;

	/// Every wave after the first (BurstAgain). Empty = Burst again.
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave")
	TObjectPtr<UAnimMontage> NextWaveMontage;

	/// Notify event in both montages that drops a ring
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave", meta = (Categories = "Enemy.Event"))
	FGameplayTag WaveEventTag;

	/// The ring to spawn (BP_Aldegon_DeathWaveRing, with the effect on it)
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave")
	TSubclassOf<AGodsentDeathWaveRing> RingClass;

	// ---- Stats (CT_AldegonStats rows) ----

	/// Damage per ring that hits
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave|Stats")
	FScalableFloat Damage = 1.f;

	/// How fast the ring grows (cm/s). Not changed by frenzy, so the dodge timing stays the same.
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave|Stats")
	FScalableFloat RingSpeed = 1.f;

	/// How far it goes (cm). Wide enough to cover the arena if you like.
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave|Stats")
	FScalableFloat MaxRadius = 1.f;

	/// How thick the ring is (cm): the band that actually hits
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave|Stats")
	FScalableFloat RingThickness = 1.f;

	/// Feet this high above the ground as it passes = jumped over it (cm)
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave|Stats")
	FScalableFloat HopClearance = 1.f;

	/// Pause between one wave's montage ending and the next starting (s). The main difficulty knob.
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave|Stats")
	FScalableFloat WaveSpacing = 1.f;

	// ---- Rules ----

	/// Skips the target's defence
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave|Rules")
	bool bMagicDamage = false;

	/// A ring hit staggers the target
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave|Rules")
	bool bCausesHitStun = false;

	/// What counts as the floor the ring is dropped on
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave|Rules")
	TEnumAsByte<ECollisionChannel> GroundObjectType = ECC_WorldStatic;

	/// How far down to look for the floor under him (cm)
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave|Rules", meta = (ClampMin = "0.0", Units = "cm"))
	float GroundSearchDistance = 1000.f;

private:
	/// Plays the next wave's montage (Burst for the first, BurstAgain after)
	void PlayNextWave();

	UFUNCTION()
	void OnWaveEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnWaveMontageDone();

	UFUNCTION()
	void OnWaveMontageCancelled();

	UFUNCTION()
	void OnSpacingFinished();

	void SpawnRing();

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> CurrentMontageTask;

	int32 TotalWaves = 1;
	int32 WavesStarted = 0;

	/// The wave whose montage end has been handled (Blend Out and Completed both fire)
	int32 WavesHandled = 0;
};
