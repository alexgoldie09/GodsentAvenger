// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GodsentShakeComponent.generated.h"

class UCurveFloat;
class USceneComponent;

/**
 * Gives a mesh a short shudder: jitters its position and rotation and gives it a quick scale punch, then settles back.
 * The bone totem shakes on every Life Siphon tick (paired with its VFX later).
 * Shakes the component tagged Shake Target Tag, or the first Static Mesh if none is tagged.
 * Only ticks while shaking.
 */
UCLASS(ClassGroup = (Godsent), meta = (BlueprintSpawnableComponent))
class GODSENTAVENGER_API UGodsentShakeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGodsentShakeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/// Starts (or restarts) a shake. Strength scales every amplitude (1 = as set below).
	UFUNCTION(BlueprintCallable, Category = "Godsent|Shake")
	void Shake(float Strength = 1.f);

	/// Stops straight away and puts the mesh back
	UFUNCTION(BlueprintCallable, Category = "Godsent|Shake")
	void StopShake();

	/// Shake this instead of the tagged / first static mesh
	UFUNCTION(BlueprintCallable, Category = "Godsent|Shake")
	void SetShakeTarget(USceneComponent* NewTarget);

	UFUNCTION(BlueprintPure, Category = "Godsent|Shake")
	bool IsShaking() const { return bShaking; }

protected:
	virtual void BeginPlay() override;

	/// Component tag that marks what to shake (add it to the mesh's Tags). None tagged = the first Static Mesh.
	UPROPERTY(EditDefaultsOnly, Category = "Shake")
	FName ShakeTargetTag = TEXT("Shake");

	/// How long one shake lasts (s)
	UPROPERTY(EditDefaultsOnly, Category = "Shake", meta = (ClampMin = "0.01", Units = "s"))
	float Duration = 0.4f;

	/// How fast it jitters (wobbles per second)
	UPROPERTY(EditDefaultsOnly, Category = "Shake", meta = (ClampMin = "0.0"))
	float Frequency = 25.f;

	/// How far it jitters side to side and up and down (cm)
	UPROPERTY(EditDefaultsOnly, Category = "Shake", meta = (ClampMin = "0.0", Units = "cm"))
	FVector PositionAmplitude = FVector(2.f, 2.f, 1.f);

	/// How far it tilts and twists (degrees)
	UPROPERTY(EditDefaultsOnly, Category = "Shake", meta = (ClampMin = "0.0", Units = "deg"))
	FRotator RotationAmplitude = FRotator(3.f, 4.f, 3.f);

	/// Quick swell in size, peaking halfway through (0.06 = 6% bigger). 0 = none.
	UPROPERTY(EditDefaultsOnly, Category = "Shake", meta = (ClampMin = "0.0"))
	float ScalePunch = 0.06f;

	/// How the jitter fades over the shake (0-1 across, 0-1 strength). Empty = a straight fade out.
	UPROPERTY(EditDefaultsOnly, Category = "Shake")
	TObjectPtr<UCurveFloat> FadeCurve;

private:
	/// Finds what to shake (tagged component, else the first static mesh)
	void FindShakeTarget();

	/// Puts the target back where it started
	void RestoreRestTransform();

	TWeakObjectPtr<USceneComponent> ShakeTarget;

	/// The target's relative transform before shaking
	FTransform RestTransform = FTransform::Identity;

	float ElapsedTime = 0.f;
	float ShakeStrength = 1.f;

	/// Random start point in the noise, so two totems never shake in sync
	float NoiseSeed = 0.f;

	bool bShaking = false;
};
