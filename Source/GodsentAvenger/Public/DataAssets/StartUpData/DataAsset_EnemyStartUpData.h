// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "DataAssets/StartUpData/DataAsset_StartUpDataBase.h"
#include "DataAsset_EnemyStartUpData.generated.h"

class UGodsentEnemyGameplayAbility;

/**
 * An enemy's startup kit: the shared startup data plus its combat abilities (activated by the behaviour tree).
 */
UCLASS()
class GODSENTAVENGER_API UDataAsset_EnemyStartUpData : public UDataAsset_StartUpDataBase
{
	GENERATED_BODY()

public:
	virtual void GiveToAbilitySystemComponent(UGodsentAbilitySystemComponent* InASCToGive, int32 ApplyLevel = 1) override;

private:
	/// Attacks activated by the behaviour tree through TryActivateAbilityByTag
	UPROPERTY(EditDefaultsOnly, Category = "StartUpData")
	TArray<TSubclassOf<UGodsentEnemyGameplayAbility>> EnemyCombatAbilities;
};
