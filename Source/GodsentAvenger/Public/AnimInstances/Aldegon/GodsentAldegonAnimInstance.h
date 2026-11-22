// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "AnimInstances/GodsentCharacterAnimInstance.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "GodsentAldegonAnimInstance.generated.h"

class UGodsentAldegonComponent;

/**
 * ABP_Aldegon's parent: the shared locomotion data plus Aldegon's state.
 * Use Throne Pose picks the sitting pose (Rise's first frame) under the montage slot,
 * so getting up and sitting down blend in and out of it cleanly.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentAldegonAnimInstance : public UGodsentCharacterAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Aldegon")
	EGodsentAldegonState AldegonState = EGodsentAldegonState::Dormant;

	/// Sitting on the throne, or sitting back down onto it
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Aldegon")
	bool bUseThronePose = true;

	/// Capsule off the ground (taking off, airborne, landing, flying home)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Aldegon")
	bool bIsFlying = false;

	/// A right-hand claw is playing: drives the Mirror node after the slot (ClawLeft played mirrored)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|Aldegon")
	bool bMirrorAttack = false;

private:
	UPROPERTY()
	TObjectPtr<UGodsentAldegonComponent> AldegonComponent;
};
