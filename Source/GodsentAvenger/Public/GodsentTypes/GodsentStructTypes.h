// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "GodsentStructTypes.generated.h"

class UGodsentHeroGameplayAbility;

/// An ability granted to the hero together with the input tag that activates it.
/// Used by the hero startup data now, and by the weapon and skill tree in later stages.
USTRUCT(BlueprintType)
struct FGodsentHeroAbilitySet
{
	GENERATED_BODY()

	/// The input tag that activates the ability
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	/// The ability granted to the hero
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGodsentHeroGameplayAbility> AbilityToGrant;

	/// True when both the input tag and the ability are set
	bool IsValid() const;
};
