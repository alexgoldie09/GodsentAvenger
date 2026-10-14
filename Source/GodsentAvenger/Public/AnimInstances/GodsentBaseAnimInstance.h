// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "GodsentBaseAnimInstance.generated.h"

/**
 * Base anim instance with a thread-safe gameplay tag check for anim graphs.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentBaseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	/// Checks whether the owning pawn has a gameplay tag
	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	bool DoesOwnerHaveTag(FGameplayTag TagToCheck) const;
};
