// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "AnimInstances/GodsentCharacterAnimInstance.h"
#include "GodsentHeroAnimInstance.generated.h"

class AGodsentHeroCharacter;

/**
 * The hero's anim instance; adds the idle "relax" state.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentHeroAnimInstance : public UGodsentCharacterAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|References")
	AGodsentHeroCharacter* OwningHeroCharacter;

	/// True once the hero has stood still for EnterRelaxStateThreshold seconds
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|LocomotionData")
	bool bShouldEnterRelaxState;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimData|LocomotionData")
	float EnterRelaxStateThreshold = 5.f;

	float IdleElapsedTime = 0.f;
};
