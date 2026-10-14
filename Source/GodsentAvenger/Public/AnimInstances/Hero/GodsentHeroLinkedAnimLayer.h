// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "AnimInstances/GodsentBaseAnimInstance.h"
#include "GodsentHeroLinkedAnimLayer.generated.h"

class UGodsentHeroAnimInstance;

/**
 * Parent class for the weapon's linked anim layer.
 */
UCLASS()
class GODSENTAVENGER_API UGodsentHeroLinkedAnimLayer : public UGodsentBaseAnimInstance
{
	GENERATED_BODY()

public:
	/// The hero anim instance this layer is linked into
	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	UGodsentHeroAnimInstance* GetHeroAnimInstance() const;
};
