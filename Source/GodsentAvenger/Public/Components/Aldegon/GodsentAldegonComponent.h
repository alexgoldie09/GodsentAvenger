// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "ActiveGameplayEffectHandle.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "GodsentTypes/GodsentStructTypes.h"
#include "GodsentAldegonComponent.generated.h"

class ACharacter;
class UAnimMontage;
class UGodsentFlightComponent;
class UGodsentHoverComponent;
class UGameplayEffect;
struct FAbilityEndedData;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAldegonStateChanged, EGodsentAldegonState, NewState, EGodsentAldegonState, OldState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAldegonFightStarted, AActor*, Challenger);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAldegonFightEnded, EGodsentBossFightEnd, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAldegonFrenzyStarted);

/**
 * Aldegon's body: which state he's in and how he moves between them (BP_Aldegon).
 * Needs a Godsent Flight component and a Godsent Hover component on the same actor (the Mesh tagged "Hover").
 *
 * Dormant (sitting) → Wake → Rise montage → float up → Take Off → Airborne.
 * Airborne ↔ Grounded through Land At / Take Off. Fly To moves him around while airborne.
 * Return Home (player left the arena): fly back, land, turn to the throne, Rise backwards, heal to full → Dormant.
 * Knocked out of the air (a knockback launch): he lands wherever he falls and is Grounded.
 *
 * The fight (Stage 18): the Boss Arena calls Start Fight / End Fight. While it's on he tracks what his behaviour tree
 * needs to decide what to do: health bands crossed, claws in a row, his living summons, frenzy.
 * The rules themselves live in the BT service Aldegon State Check.
 *
 * Defeat (Stage 21): his death saves Boss Tag as beaten (the main menu's star) and, if Win Run On Defeat is on,
 * wins the run (the game mode shows the win screen).
 *
 * Summons (Stage 20): the Summon ability registers what it spawns here. Max Adds / Max Totems are his caps at full health,
 * and the Add / Totem Cap Steps raise them as his health drops. Health bands only count the first time he drops past them,
 * so a totem healing him back up can't make him lift off twice for the same band.
 *
 * Every tuning number is a Scalable Float, so it can point at a row in CT_AldegonStats.
 * New stats start at Value 1 (so picking a row just works) and log a warning if they aren't linked to a table.
 */
UCLASS(ClassGroup = (Godsent), meta = (BlueprintSpawnableComponent))
class GODSENTAVENGER_API UGodsentAldegonComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGodsentAldegonComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/// Dormant only: gets up off the throne, then takes off
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon")
	bool Wake();

	/// Grounded only: climbs up to Airborne Height
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon")
	bool TakeOff();

	/// Airborne only: flies to a spot and stays airborne
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon")
	bool FlyTo(const FVector& Destination);

	/// Airborne only: flies to a spot on the navmesh and lands there
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon")
	bool LandAt(const FVector& Spot);

	/// Any awake state: flies back to the throne, sits down and heals to full
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon")
	bool ReturnHome();

	/// Faces this actor while flying (e.g. the player). Null = face the way he's flying.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon")
	void SetFacingTarget(AActor* NewFacingTarget);

	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon")
	EGodsentAldegonState GetAldegonState() const { return CurrentState; }

	/// Flying for any reason (airborne, taking off, landing, flying home)
	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon")
	bool IsFlying() const;

	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon")
	FVector GetHomeLocation() const { return HomeLocation; }

	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon")
	FRotator GetHomeRotation() const { return HomeRotation; }

	/// Re-reads every stat from the curve table (at the enemy's applied level) and pushes them to flight and hover
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon")
	void ApplyStats();

	UPROPERTY(BlueprintAssignable, Category = "Godsent|Aldegon")
	FOnAldegonStateChanged OnAldegonStateChanged;

	// ---- Fight ----

	/// The player walked into the arena: target them and wake up (or turn back if he was on his way home)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon|Fight")
	void StartFight(AActor* NewChallenger);

	/// The player left the arena: drop the target, clear his summons and go home to heal
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon|Fight")
	void EndFight();

	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Fight")
	bool IsFightActive() const { return bFightActive; }

	/// Who he's fighting (null outside a fight)
	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Fight")
	AActor* GetChallenger() const { return Challenger.Get(); }

	/// Fight started: show the boss bar here
	UPROPERTY(BlueprintAssignable, Category = "Godsent|Aldegon|Fight")
	FOnAldegonFightStarted OnFightStarted;

	/// Fight ended (player left, or he died): hide the boss bar here
	UPROPERTY(BlueprintAssignable, Category = "Godsent|Aldegon|Fight")
	FOnAldegonFightEnded OnFightEnded;

	/// Health dropped to Frenzy At (his claws become unblockable)
	UPROPERTY(BlueprintAssignable, Category = "Godsent|Aldegon|Fight")
	FOnAldegonFrenzyStarted OnFrenzyStarted;

	// ---- What the behaviour tree reads ----

	/// A health band was crossed while grounded: he should lift off
	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Fight")
	bool IsLiftOffPending() const { return bLiftOffPending; }

	/// Enough health bands were crossed while airborne: he should come down
	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Fight")
	bool IsForcedLandingPending() const;

	/// Claws since his last lift-off, Soul Beam or Death Wave
	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Fight")
	int32 GetConsecutiveClaws() const { return ConsecutiveClaws; }

	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Fight")
	bool IsFrenzied() const { return bFrenzied; }

	/// Knocked back: from the launch until Knockdown Recovery Time after he lands. No moves or flight orders.
	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Fight")
	bool IsKnockedDown() const { return bKnockedDown; }

	/// Seconds since his state last changed
	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Fight")
	float GetTimeInState() const;

	/// 0-100
	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Fight")
	float GetHealthPercent() const;

	/// Ground Trigger Walk Seconds × the target's normal walk speed (cm)
	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Fight")
	float GetGroundTriggerDistance(const AActor* Target) const;

	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Stats")
	float GetResummonDistance() const;

	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Stats")
	float GetMinAirborneTime() const;

	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Stats")
	int32 GetMaxConsecutiveClaws() const;

	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Stats")
	float GetClawRange() const;

	// ---- Summons ----

	/// Call when he summons something (the Summon ability does this for you). It counts towards his cap until it dies.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon|Summons")
	void RegisterSummon(AActor* Summon, EGodsentAldegonSummonType SummonType);

	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Summons")
	int32 GetLivingSummonCount(EGodsentAldegonSummonType SummonType) const;

	/// Max Adds or Max Totems, raised by the Add / Totem Cap Steps his health has reached
	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Summons")
	int32 GetMaxSummons(EGodsentAldegonSummonType SummonType) const;

	/// Every living add and totem
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon|Summons")
	void GetLivingSummons(TArray<AActor*>& OutSummons) const;

	/// Under the cap for that kind and its ability could activate right now (cooldown, cost, blocking tags).
	/// No Ability Tag = his own Summon / Life Siphon tag for that kind.
	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Summons")
	bool CanSummonNow(EGodsentAldegonSummonType SummonType, FGameplayTag AbilityTag) const;

	/// Kills every add and totem he summoned (his death, or the player leaving)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon|Summons")
	void KillAllSummons();

	// ---- Moves (Stage 19) ----

	/// Call as a claw starts (before the melee base runs). Picks the hand: random on the first claw of a streak,
	/// then always the other hand. Mirrors the animation and hand box for the right hand, and makes it unblockable in frenzy.
	/// Returns true for the right hand (mirrored).
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon|Moves")
	bool BeginClaw();

	/// Call as the claw ends: puts the mirror, hand boxes and unblockable tag back
	UFUNCTION(BlueprintCallable, Category = "Godsent|Aldegon|Moves")
	void EndClaw();

	/// ABP_Aldegon's Mirror node reads this
	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Moves")
	bool IsClawMirrored() const { return bClawMirrored; }

	/// How many Death Waves in a row at his current health: 1 + each Death Wave Threshold he's at or below, capped by Max Death Waves
	UFUNCTION(BlueprintPure, Category = "Godsent|Aldegon|Moves")
	int32 GetDeathWaveCount() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ---- Stats (point these at CT_Aldegon rows) ----

	/// How high his feet are when airborne (what you see, Grounded Height float included). "A head above the player."
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats")
	FScalableFloat AirborneHeight = 250.f;

	/// How far his feet float off the floor (always on while he's awake, airborne or grounded). "Inches."
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats")
	FScalableFloat GroundedHeight = 15.f;

	/// Top flying speed (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats")
	FScalableFloat FlySpeed = 350.f;

	/// Top climbing speed (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats")
	FScalableFloat ClimbSpeed = 200.f;

	/// Top coming-down speed (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats")
	FScalableFloat DescentSpeed = 250.f;

	/// Turning speed while flying (degrees/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats")
	FScalableFloat FlyTurnRate = 120.f;

	// ---- Fight stats ----

	/// Airborne: the player within this many seconds of walking makes him come down ("2-3 seconds of default walking")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats|Fight")
	FScalableFloat GroundTriggerWalkSeconds = 1.f;

	/// Grounded with no adds or totems left: the player further than this makes him go back up to resummon (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats|Fight")
	FScalableFloat ResummonDistance = 1.f;

	/// After taking off he stays up at least this long before the player getting close brings him down (s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats|Fight")
	FScalableFloat MinAirborneTime = 1.f;

	/// After a knockback lands him, he waits this long before doing anything (s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats|Fight")
	FScalableFloat KnockdownRecoveryTime = 1.f;

	/// Claws in a row before he lifts off. 0 = no limit.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats|Fight")
	FScalableFloat MaxConsecutiveClaws = 1.f;

	/// Health bands crossed while airborne that force him down. 0 = never.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats|Fight")
	FScalableFloat AirborneBandsToLand = 1.f;

	/// Health % at which he frenzies (unblockable claws). 0 = never.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats|Fight")
	FScalableFloat FrenzyAtHealthPercent = 1.f;

	/// Most zombies and ghosts alive at once (at full health; Add Cap Steps raise it)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats|Fight")
	FScalableFloat MaxAdds = 1.f;

	/// Most Life Siphon totems alive at once (at full health; Totem Cap Steps raise it)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats|Fight")
	FScalableFloat MaxTotems = 1.f;

	/// How close the player must be for him to Claw (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats|Fight")
	FScalableFloat ClawRange = 1.f;

	/// Most Death Waves in one combo (Andre: "up to 3 times")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Stats|Fight")
	FScalableFloat MaxDeathWaves = 1.f;

	// ---- Fight rules (not per level, so these live here rather than the curve table) ----

	/// Health % checkpoints. Crossing one while grounded makes him lift off. Add, remove or space them however you like.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Fight")
	TArray<float> LiftOffHealthBands = { 90.f, 80.f, 70.f, 60.f, 50.f, 40.f, 30.f, 20.f, 10.f };

	/// Health % at which his Death Wave combo gets one wave longer (at or below each). Capped by Max Death Waves.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Fight")
	TArray<float> DeathWaveThresholds = { 75.f, 50.f };

	/// More adds allowed as his health drops: at or below each step's health %, up to its Max Alive (the highest reached wins)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Fight")
	TArray<FGodsentAldegonCapStep> AddCapSteps;

	/// The same for totems
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Fight")
	TArray<FGodsentAldegonCapStep> TotemCapSteps;

	/// Applied while frenzied (GE_Aldegon_Frenzy: raises Attack Speed, so claws, Death Wave and summons come faster)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Fight")
	TSubclassOf<UGameplayEffect> FrenzyEffect;

	/// Ending one of these counts as a Claw
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Fight", meta = (Categories = "Enemy.Ability"))
	FGameplayTag ClawAbilityTag;

	/// The ability that summons adds (checked for cooldown before going up to resummon)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Fight", meta = (Categories = "Enemy.Ability"))
	FGameplayTag SummonAbilityTag;

	/// The ability that summons totems
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Fight", meta = (Categories = "Enemy.Ability"))
	FGameplayTag LifeSiphonAbilityTag;

	/// Moves with any of these are cancelled when the player leaves the arena (so he doesn't fly home mid-Beam)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Fight", meta = (Categories = "Enemy.Ability"))
	FGameplayTagContainer CancelOnFightEndTags;

	/// Moves with any of these are cancelled when he's knocked back (launched off the ground or out of the air)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Fight", meta = (Categories = "Enemy.Ability"))
	FGameplayTagContainer CancelOnKnockbackTags;

	// ---- Defeat (Stage 21) ----

	/// Saved when he dies, so the main menu can show he's been beaten (survives New Game)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Defeat", meta = (Categories = "GameData.Boss"))
	FGameplayTag BossTag;

	/// His death wins the run (the win screen shows after the game mode's Win Screen Delay). Off for a boss that isn't the end of a level.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Defeat")
	bool bWinRunOnDefeat = true;

	/// Ending one of these resets the Claw count (anything that isn't a claw breaks the streak)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Fight", meta = (Categories = "Enemy.Ability"))
	FGameplayTagContainer ClawResetAbilityTags;

	// ---- Throne ----

	/// Getting out of the throne. Played backwards to sit back down.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne")
	TObjectPtr<UAnimMontage> RiseMontage;

	/// Play rate for Rise when getting up
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "0.1"))
	float RisePlayRate = 1.f;

	/// Play rate for Rise backwards when sitting down (positive; it's flipped for you)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "0.1"))
	float SitDownPlayRate = 1.f;

	/// How far through Rise (0-1) he starts floating, so he drifts up as he finishes standing
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RiseHoverStart = 0.7f;

	/// How far through sitting down (0-1) the float settles, so he's resting before he touches the seat
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SitDownSettleAt = 0.f;

	/// Speed he glides onto the throne after landing beside it (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "1.0", Units = "cm/s"))
	float HomeSlideSpeed = 150.f;

	/// Counts as back on the throne this close to where he was placed
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "0.0", Units = "cm"))
	float HomeArriveDistance = 2.f;

	/// Blend out time if Rise is stopped part-way (sent home while still getting up)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "0.0", Units = "s"))
	float MontageStopBlendTime = 0.25f;

	/// Turning speed when lining up with the throne after landing at home (degrees/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "1.0"))
	float HomeTurnRate = 180.f;

	/// Counts as lined up with the throne within this many degrees
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne", meta = (ClampMin = "0.0"))
	float HomeFacingTolerance = 2.f;

	/// Back to full health once he's sat back down
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aldegon|Throne")
	bool bHealOnReturnHome = true;

	/// Logs every state change to the output log
	UPROPERTY(EditDefaultsOnly, Category = "Aldegon|Debug")
	bool bLogStateChanges = true;

private:
	void SetState(EGodsentAldegonState NewState);

	/// Enemy level the stats are read at
	float GetStatLevel() const;

	/// Plays Rise forwards (getting up) or backwards (sitting down). Returns the montage length in seconds (0 = didn't play).
	float PlayRise(bool bBackwards);

	void OnRiseFinished();
	void OnSitDownFinished();

	/// Lines up with the throne after landing at home, then sits
	void TickTurnToHome(float DeltaTime);

	void HealToFull();

	UFUNCTION()
	void OnFlightActionFinished(EGodsentFlightAction Action, bool bSucceeded);

	UFUNCTION()
	void OnMovementModeChanged(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

	void OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount);

	/// Health bands and frenzy
	void OnHealthChanged(const FOnAttributeChangeData& Data);

	/// Claw counting
	void OnAbilityEnded(const FAbilityEndedData& AbilityEndedData);

	/// The player came back while he was flying home: turn round and fight
	void ResumeFromReturnHome();

	/// Clears bands, claws and frenzy (after healing up on the throne)
	void ResetFightCounters();

	void SetFrenzy(bool bNewFrenzied);

	/// Launched: cancel his move and block new ones until he's recovered
	void StartKnockdown();

	/// Recovered (or the fight ended / he died): moves allowed again
	void EndKnockdown();

	/// Reads a stat at his level, warning once if it isn't linked to a curve table
	float ReadStat(const FScalableFloat& Stat, const TCHAR* StatName) const;

	/// Living summons of one kind (dead and destroyed ones are dropped)
	TArray<TWeakObjectPtr<AActor>>& GetSummonList(EGodsentAldegonSummonType SummonType);
	const TArray<TWeakObjectPtr<AActor>>& GetSummonList(EGodsentAldegonSummonType SummonType) const;

	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UGodsentFlightComponent> FlightComponent;

	UPROPERTY()
	TObjectPtr<UGodsentHoverComponent> HoverComponent;

	EGodsentAldegonState CurrentState = EGodsentAldegonState::Dormant;

	FVector HomeLocation = FVector::ZeroVector;
	FRotator HomeRotation = FRotator::ZeroRotator;

	/// Landed at home and turning to face the throne
	bool bTurningToHome = false;

	FTimerHandle RiseTimerHandle;
	FTimerHandle HoverTimerHandle;
	FTimerHandle KnockdownTimerHandle;

	bool bKnockedDown = false;

	FDelegateHandle DeadTagDelegateHandle;
	FDelegateHandle HealthChangedDelegateHandle;
	FDelegateHandle AbilityEndedDelegateHandle;

	// ---- Fight ----

	bool bFightActive = false;
	TWeakObjectPtr<AActor> Challenger;

	bool bLiftOffPending = false;
	int32 AirborneBandsCrossed = 0;
	int32 ConsecutiveClaws = 0;
	bool bFrenzied = false;

	/// Lowest health % this fight (a band only counts the first time he drops past it, even if a totem heals him back above)
	float LowestHealthPercent = 100.f;

	/// World time his state last changed
	float StateEnteredTime = 0.f;

	/// Claw hands: the last claw was the right (mirrored) one
	bool bLastClawMirrored = false;
	bool bClawMirrored = false;

	/// Added Unblockable for a frenzied claw (so End Claw only removes what Begin Claw added)
	bool bClawAddedUnblockable = false;

	FActiveGameplayEffectHandle FrenzyEffectHandle;

	TArray<TWeakObjectPtr<AActor>> LivingAdds;
	TArray<TWeakObjectPtr<AActor>> LivingTotems;

	/// Stats already warned about (so the log isn't spammed)
	mutable TSet<FName> WarnedUnlinkedStats;
};