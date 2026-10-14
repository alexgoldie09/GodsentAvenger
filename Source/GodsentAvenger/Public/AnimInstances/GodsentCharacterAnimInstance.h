// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "AnimInstances/GodsentBaseAnimInstance.h"
#include "GodsentCharacterAnimInstance.generated.h"

class AGodsentBaseCharacter;
class UCharacterMovementComponent;

/**
 * Locomotion data shared by the hero and enemy anim blueprints.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentCharacterAnimInstance : public UGodsentBaseAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY()
	AGodsentBaseCharacter* OwningCharacter;

	UPROPERTY()
	UCharacterMovementComponent* OwningMovementComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|LocomotionData")
	float GroundSpeed;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|LocomotionData")
	bool bHasAcceleration;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "AnimData|LocomotionData")
	float LocomotionDirection;
};
