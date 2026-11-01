// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GodsentHoverComponent.generated.h"

/**
 * Procedural float for meshes without a skeleton (the ghost's skull).
 * Moves one "visual root" scene component (found by its component tag, default "Hover") relative to where it was placed:
 * - Hover Height lifts it (or just place the visual root where you want it in the Blueprint viewport);
 * - idle bob up and down;
 * - while moving: sways side to side, leans into its movement and banks when strafing;
 * - Agitation (0-1) adds a building shake (Mindfire charge);
 * - Play Recoil kicks it back and up, then springs back (Mindfire fire).
 * The capsule keeps walking on the navmesh with gravity; only the visuals float.
 */
UCLASS(ClassGroup = (Godsent), meta = (BlueprintSpawnableComponent))
class GODSENTAVENGER_API UGodsentHoverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGodsentHoverComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/// 0 = calm, 1 = full shake. Eases towards the new value.
	UFUNCTION(BlueprintCallable, Category = "Godsent|Hover")
	void SetAgitation(float NewAgitation);

	/// Kicks the visuals back and up (Strength 1 = Recoil Distance), then springs back
	UFUNCTION(BlueprintCallable, Category = "Godsent|Hover")
	void PlayRecoil(float Strength = 1.f);

	/// Uses a different scene component as the visual root (instead of the tagged one)
	UFUNCTION(BlueprintCallable, Category = "Godsent|Hover")
	void SetVisualRoot(USceneComponent* NewVisualRoot);

	UFUNCTION(BlueprintPure, Category = "Godsent|Hover")
	USceneComponent* GetVisualRoot() const { return VisualRoot.Get(); }

protected:
	virtual void BeginPlay() override;

	/// Component tag on the scene component to move (put the skull and the aura under it)
	UPROPERTY(EditDefaultsOnly, Category = "Hover")
	FName VisualRootTag = FName("Hover");

	/// Extra height on top of where the visual root sits in the Blueprint
	UPROPERTY(EditDefaultsOnly, Category = "Hover", meta = (Units = "cm"))
	float HoverHeight = 0.f;

	// ---- Idle bob ----

	UPROPERTY(EditDefaultsOnly, Category = "Hover|Bob", meta = (ClampMin = "0.0", Units = "cm"))
	float BobAmplitude = 8.f;

	/// Bobs per second
	UPROPERTY(EditDefaultsOnly, Category = "Hover|Bob", meta = (ClampMin = "0.0"))
	float BobFrequency = 0.5f;

	/// Bob size while moving at full speed (0.5 = half the idle bob)
	UPROPERTY(EditDefaultsOnly, Category = "Hover|Bob", meta = (ClampMin = "0.0"))
	float MovingBobScale = 0.5f;

	// ---- Movement sway ----

	/// Speed at which sway and lean reach full strength
	UPROPERTY(EditDefaultsOnly, Category = "Hover|Sway", meta = (ClampMin = "1.0", Units = "cm/s"))
	float FullSwaySpeed = 450.f;

	/// Side-to-side drift while moving
	UPROPERTY(EditDefaultsOnly, Category = "Hover|Sway", meta = (ClampMin = "0.0", Units = "cm"))
	float SwayDistance = 10.f;

	/// Side-to-side roll while moving
	UPROPERTY(EditDefaultsOnly, Category = "Hover|Sway", meta = (ClampMin = "0.0", Units = "deg"))
	float SwayRoll = 10.f;

	/// Sways per second
	UPROPERTY(EditDefaultsOnly, Category = "Hover|Sway", meta = (ClampMin = "0.0"))
	float SwayFrequency = 0.9f;

	/// Tips forward when moving forward (and back when retreating)
	UPROPERTY(EditDefaultsOnly, Category = "Hover|Sway", meta = (ClampMin = "0.0", Units = "deg"))
	float MaxLeanPitch = 10.f;

	/// Banks into sideways movement (strafing while facing the player)
	UPROPERTY(EditDefaultsOnly, Category = "Hover|Sway", meta = (ClampMin = "0.0", Units = "deg"))
	float MaxBankRoll = 15.f;

	/// How quickly lean and bank follow the movement
	UPROPERTY(EditDefaultsOnly, Category = "Hover|Sway", meta = (ClampMin = "0.0"))
	float LeanInterpSpeed = 4.f;

	// ---- Agitation (charge shake) ----

	UPROPERTY(EditDefaultsOnly, Category = "Hover|Agitation", meta = (ClampMin = "0.0", Units = "cm"))
	float AgitationShakeDistance = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hover|Agitation", meta = (ClampMin = "0.0", Units = "deg"))
	float AgitationShakeRoll = 6.f;

	/// Extra bob speed at full agitation (1 = double)
	UPROPERTY(EditDefaultsOnly, Category = "Hover|Agitation", meta = (ClampMin = "0.0"))
	float AgitationBobSpeedUp = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hover|Agitation", meta = (ClampMin = "0.0"))
	float AgitationInterpSpeed = 3.f;

	// ---- Recoil ----

	UPROPERTY(EditDefaultsOnly, Category = "Hover|Recoil", meta = (ClampMin = "0.0", Units = "cm"))
	float RecoilDistance = 25.f;

	UPROPERTY(EditDefaultsOnly, Category = "Hover|Recoil", meta = (ClampMin = "0.0", Units = "deg"))
	float RecoilPitch = 15.f;

	/// How quickly it springs back after a recoil
	UPROPERTY(EditDefaultsOnly, Category = "Hover|Recoil", meta = (ClampMin = "0.0"))
	float RecoilRecoverySpeed = 5.f;

private:
	TWeakObjectPtr<USceneComponent> VisualRoot;

	FVector BaseRelativeLocation = FVector::ZeroVector;
	FRotator BaseRelativeRotation = FRotator::ZeroRotator;

	float BobPhase = 0.f;
	float SwayPhase = 0.f;
	float ShakeTime = 0.f;

	float CurrentAgitation = 0.f;
	float TargetAgitation = 0.f;
	float RecoilAlpha = 0.f;

	float CurrentLeanPitch = 0.f;
	float CurrentBankRoll = 0.f;
};
