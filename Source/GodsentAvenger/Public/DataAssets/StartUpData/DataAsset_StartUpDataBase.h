// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DataAsset_StartUpDataBase.generated.h"

class UGodsentGameplayAbility;
class UGodsentAbilitySystemComponent;
class UGameplayEffect;

/**
 * Abilities and effects given to a character when it's possessed.
 */
UCLASS()
class GODSENTAVENGER_API UDataAsset_StartUpDataBase : public UDataAsset
{
	GENERATED_BODY()

public:
	/// Grants the abilities and applies the effects below
	virtual void GiveToAbilitySystemComponent(UGodsentAbilitySystemComponent* InASCToGive, int32 ApplyLevel = 1);

protected:
	/// Abilities that activate as soon as they're granted (e.g. equip the sword)
	UPROPERTY(EditDefaultsOnly, Category = "StartUpData")
	TArray<TSubclassOf<UGodsentGameplayAbility>> ActivateOnGivenAbilities;

	/// Abilities triggered by gameplay events rather than input (hit react, death)
	UPROPERTY(EditDefaultsOnly, Category = "StartUpData")
	TArray<TSubclassOf<UGodsentGameplayAbility>> ReactiveAbilities;

	/// Effects applied once at startup (stat initialisation, stamina regen)
	UPROPERTY(EditDefaultsOnly, Category = "StartUpData")
	TArray<TSubclassOf<UGameplayEffect>> StartUpGameplayEffects;

	/// Grants a list of abilities at a level
	void GrantAbilities(const TArray<TSubclassOf<UGodsentGameplayAbility>>& AbilitiesToGive, UGodsentAbilitySystemComponent* InASCToGive, int32 ApplyLevel);
};
