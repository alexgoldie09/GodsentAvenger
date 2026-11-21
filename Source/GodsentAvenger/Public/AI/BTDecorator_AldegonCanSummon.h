// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BTDecorator.h"
#include "GodsentTypes/GodsentEnumTypes.h"
#include "BTDecorator_AldegonCanSummon.generated.h"

/**
 * Passes while Aldegon can summon more of one kind: under his cap (Max Adds or Max Totems) and the ability is ready
 * (not on cooldown, not blocked). When both Summon and Life Siphon fail this, the Airborne branch falls through to
 * "out of options": fly away and land.
 */
UCLASS(meta = (DisplayName = "Aldegon Can Summon"))
class GODSENTAVENGER_API UBTDecorator_AldegonCanSummon : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_AldegonCanSummon();

	//~ Begin UBTDecorator Interface
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
	//~ End UBTDecorator Interface

protected:
	/// Adds (zombies and ghosts) or Totems
	UPROPERTY(EditAnywhere, Category = "Summon")
	EGodsentAldegonSummonType SummonType = EGodsentAldegonSummonType::Add;

	/// The ability that summons them, checked for cooldown. Empty = Aldegon's own Summon / Life Siphon tag for that kind.
	UPROPERTY(EditAnywhere, Category = "Summon", meta = (Categories = "Enemy.Ability"))
	FGameplayTag AbilityTag;
};
