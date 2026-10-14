// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "DataAssets/StartUpData/DataAsset_StartUpDataBase.h"
#include "GodsentTypes/GodsentStructTypes.h"
#include "DataAsset_HeroStartUpData.generated.h"

/**
 * The hero's baseline kit. Skill tree abilities are granted by the skill tree later, not here.
 */
UCLASS()
class GODSENTAVENGER_API UDataAsset_HeroStartUpData : public UDataAsset_StartUpDataBase
{
	GENERATED_BODY()

public:
	virtual void GiveToAbilitySystemComponent(UGodsentAbilitySystemComponent* InASCToGive, int32 ApplyLevel = 1) override;

private:
	/// Input-bound abilities every heir starts with (e.g. target lock)
	UPROPERTY(EditDefaultsOnly, Category = "StartUpData", meta = (TitleProperty = "InputTag"))
	TArray<FGodsentHeroAbilitySet> HeroStartUpAbilitySets;
};
