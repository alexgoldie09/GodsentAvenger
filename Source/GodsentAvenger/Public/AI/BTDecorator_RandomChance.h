// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_RandomChance.generated.h"

/**
 * Lets its branch run only Chance of the time (rolled each time the branch is tried).
 * Under a Selector this picks between attacks: [Heavy, Chance 0.35] then [Light] = 35% heavy, 65% light.
 */
UCLASS()
class GODSENTAVENGER_API UBTDecorator_RandomChance : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_RandomChance();

protected:
	//~ Begin UBTDecorator Interface
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	//~ End UBTDecorator Interface

	virtual FString GetStaticDescription() const override;

	/// 0 = never, 1 = always
	UPROPERTY(EditAnywhere, Category = "Random", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Chance = 0.5f;
};
