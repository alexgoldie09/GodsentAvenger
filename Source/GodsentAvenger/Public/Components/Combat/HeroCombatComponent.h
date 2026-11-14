// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/Combat/PawnCombatComponent.h"
#include "HeroCombatComponent.generated.h"

class AGodsentHeroWeapon;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMeshComponent;

/**
 * The hero's combat component: weapon lookups and melee hit events.
 *
 * Chaste Cleave: the sword attack calls Try Start Cleave Swing at the start of each swing. If the hero has
 * Player.Passive.Cleave and the chance roll passes, that swing is a cleave:
 *  - a holy echo of the sword (Cleave Echo Material) grows out to the cleave's reach as the tell, then fades
 *    once the swing ends. With Hide Sword During Cleave the sword's materials are swapped for the echo (only the
 *    echo shows), otherwise the echo is drawn over the sword as its Overlay Material.
 *  - while the weapon's hit box is on, the blade is extended by Extra Length, thickened to Width and swept
 *    from last frame's position to this frame's, so it follows the real swing (nothing behind the hero is hit)
 *  - every hostile it passes through gets the normal melee hit (same damage, block, parry and hit pause)
 *  - ANS_HeroTrail swaps to the cleave trail, sized to the same extended blade
 */
UCLASS()
class GODSENTAVENGER_API UHeroCombatComponent : public UPawnCombatComponent
{
	GENERATED_BODY()

public:
	UHeroCombatComponent();

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

	/// Set by the target lock ability while locked on (nullptr when not locked)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Combat")
	void SetLockedOnTarget(AActor* InLockedOnTarget) { LockedOnTarget = InLockedOnTarget; }

	/// The enemy currently locked onto, or nullptr when not locked on
	UFUNCTION(BlueprintPure, Category = "Godsent|Combat")
	AActor* GetLockedOnTarget() const { return LockedOnTarget.Get(); }

	/// Chaste Cleave: rolls Chance (0-1) for this swing. Does nothing (returns false) without Player.Passive.Cleave.
	/// Extra Length and Width (cm) are how far past the tip and how thick the swept hit area is.
	/// Call it at the start of every normal swing (not the riposte), and End Cleave Swing when the attack ends.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Combat|Cleave")
	bool TryStartCleaveSwing(float Chance, float ExtraLength, float Width);

	/// Ends the cleave (the glow fades out). Safe to call when the swing wasn't a cleave.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Combat|Cleave")
	void EndCleaveSwing();

	/// True during a swing that rolled a cleave
	UFUNCTION(BlueprintPure, Category = "Godsent|Combat|Cleave")
	bool IsCleaveSwing() const { return bCleaveSwing; }

	/// The blade as a line from the hilt end to the tip, pushed out by the cleave's Extra Length on a cleave swing.
	/// Taken from the weapon's hit box (its longest side is the blade). False with no weapon equipped.
	UFUNCTION(BlueprintPure, Category = "Godsent|Combat|Cleave")
	bool GetCleaveBladeSegment(FVector& OutStart, FVector& OutEnd) const;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void ToggleCurrentEquippedWeaponCollision(bool bShouldEnable) override;

	/// The holy echo of the sword (M_SwordEcho). Drawn over the sword as its Overlay Material only while it shows.
	UPROPERTY(EditDefaultsOnly, Category = "Cleave")
	TObjectPtr<UMaterialInterface> CleaveEchoMaterial;

	/// On: the sword turns into the echo (its materials are swapped, then put back once the echo has faded).
	/// Off: the echo is drawn over the normal sword.
	UPROPERTY(EditDefaultsOnly, Category = "Cleave")
	bool bHideSwordDuringCleave = true;

	/// Scalar on the echo material: 0 = hidden, 1 = fully out
	UPROPERTY(EditDefaultsOnly, Category = "Cleave")
	FName CleaveGlowParameterName = TEXT("CleaveGlow");

	/// Scalar on the echo material: how far it stretches along the blade, so its tip lands where the cleave reaches
	/// ((blade + Extra Length) / blade, measured from the grip)
	UPROPERTY(EditDefaultsOnly, Category = "Cleave")
	FName CleaveEchoStretchParameterName = TEXT("CleaveEchoStretch");

	/// How quickly the echo grows out at the start of a cleave swing
	UPROPERTY(EditDefaultsOnly, Category = "Cleave", meta = (ClampMin = "0.0", Units = "s"))
	float CleaveGlowFadeInTime = 0.1f;

	/// How long the echo takes to fade once the swing ends
	UPROPERTY(EditDefaultsOnly, Category = "Cleave", meta = (ClampMin = "0.0", Units = "s"))
	float CleaveGlowFadeOutTime = 0.35f;

	/// Only hit pawns in front of the hero: 0 = anything not behind, higher = a narrower front
	UPROPERTY(EditDefaultsOnly, Category = "Cleave", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float CleaveMinFrontDot = 0.f;

	/// Most sweep checks in one frame (a very fast swing or a frame hitch is split into this many at most)
	UPROPERTY(EditDefaultsOnly, Category = "Cleave", meta = (ClampMin = "1"))
	int32 MaxCleaveSweepSteps = 8;

	/// Draws the swept hit area (green) and what it hit (red)
	UPROPERTY(EditAnywhere, Category = "Cleave")
	bool bDrawCleaveDebug = false;

private:
	/// Sweeps the extended blade from last frame's position to this frame's and hits what it passes through
	void SweepCleaveBlade();

	/// One capsule along the blade at a single moment
	void OverlapCleaveCapsule(const FVector& BladeStart, const FVector& BladeEnd);

	/// Moves the echo towards on or off and sends it to the echo material
	void UpdateCleaveGlow(float DeltaTime);

	/// Puts the echo material on the sword as its overlay
	void ShowCleaveEcho();

	/// Takes the echo off the sword (nothing extra is drawn between cleaves)
	void HideCleaveEcho();

	/// Ticks only while a cleave is running or the glow is fading
	void RefreshTickEnabled();

	bool bCleaveSwing = false;
	bool bWeaponCollisionOn = false;
	float CleaveExtraLength = 0.f;
	float CleaveWidth = 0.f;
	float CleaveGlowAlpha = 0.f;
	float CleaveEchoStretch = 1.f;

	/// The echo material's instance and the sword it's on
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CleaveEchoInstance;

	TWeakObjectPtr<UMeshComponent> CleaveEchoMesh;

	/// The sword's own materials while they're swapped for the echo (Hide Sword During Cleave)
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> SwappedOutSwordMaterials;

	bool bSwordMaterialsSwapped = false;

	/// The blade last frame, so the sweep covers the gap between frames
	bool bHasPreviousBlade = false;
	FVector PreviousBladeStart = FVector::ZeroVector;
	FVector PreviousBladeEnd = FVector::ZeroVector;

	/// Weak so a parried enemy that dies or despawns doesn't linger here
	TWeakObjectPtr<AActor> RiposteTarget;

	/// Weak for the same reason; the target lock ability clears it when the lock ends
	TWeakObjectPtr<AActor> LockedOnTarget;
};
