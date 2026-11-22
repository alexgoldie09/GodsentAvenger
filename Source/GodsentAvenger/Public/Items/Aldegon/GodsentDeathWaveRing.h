// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "GodsentDeathWaveRing.generated.h"

class UAbilitySystemComponent;
class UNiagaraComponent;

/**
 * One Death Wave ring (BP_Aldegon_DeathWaveRing). It grows outwards along the ground from where it was dropped.
 * Only the ring itself hurts: you're hit while its band (Radius - Thickness to Radius) passes over you, once per ring.
 * Standing inside after it's gone past, or outside before it arrives, is safe.
 * Avoid it by jumping (Hop/Leap, feet above Hop Clearance as it passes) or dodging through it (i-frames).
 *
 * The visual uses the same radius as the hit check every frame: the Niagara effect gets Radius and Thickness
 * float parameters, and On Ring Grown fires for Blueprint visuals (e.g. scaling a ring mesh).
 */
UCLASS(Blueprintable)
class GODSENTAVENGER_API AGodsentDeathWaveRing : public AActor
{
	GENERATED_BODY()

public:
	AGodsentDeathWaveRing();

	virtual void Tick(float DeltaTime) override;

	/// Set by the Death Wave ability straight after spawning
	void InitRing(APawn* InOwnerPawn, UAbilitySystemComponent* InSourceASC, const FGameplayEffectSpecHandle& InDamageSpec,
		float InSpeed, float InMaxRadius, float InThickness, float InHopClearance);

	UFUNCTION(BlueprintPure, Category = "Godsent|Death Wave")
	float GetRadius() const { return CurrentRadius; }

protected:
	/// Every frame the ring grows (drive a mesh or material from it)
	UFUNCTION(BlueprintImplementableEvent, Category = "Death Wave", meta = (DisplayName = "On Ring Grown"))
	void BP_OnRingGrown(float RingRadius, float RingThickness);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death Wave")
	TObjectPtr<USceneComponent> RingRoot;

	/// The ring effect (placeholder now, like the Wail; the real one later)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Death Wave")
	TObjectPtr<UNiagaraComponent> RingEffect;

	/// Niagara float parameters set every frame
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave")
	FName RadiusParameter = TEXT("Radius");

	UPROPERTY(EditDefaultsOnly, Category = "Death Wave")
	FName ThicknessParameter = TEXT("Thickness");

	/// Seconds it stays after reaching full size (lets the effect fade) before it's removed
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave", meta = (ClampMin = "0.0", Units = "s"))
	float LingerTime = 0.5f;

	/// Draws the hit band (two circles) so it can be lined up with the effect
	UPROPERTY(EditDefaultsOnly, Category = "Death Wave|Debug")
	bool bDrawDebug = false;

private:
	/// Hits anyone the band is passing over
	void CheckHits();

	TWeakObjectPtr<APawn> OwnerPawn;
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	FGameplayEffectSpecHandle DamageSpec;

	float Speed = 0.f;
	float MaxRadius = 0.f;
	float Thickness = 0.f;
	float HopClearance = 0.f;

	float CurrentRadius = 0.f;
	bool bFinishedGrowing = false;

	/// Already hit by this ring (once each)
	TSet<TWeakObjectPtr<AActor>> HitActors;
};
